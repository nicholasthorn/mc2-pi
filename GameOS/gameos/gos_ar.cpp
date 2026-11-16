// The tabletop on a phone through ARCore: see gos_ar.h.
//
// ARCore's session runs on the game's GL thread: ArSession_update, once a
// frame before the world draws, latches the newest camera image into an
// external texture and gives the phone's pose. The camera image is drawn over
// the whole window (bindEye), then the world, through the one eye eyeViews
// gives, in the table's frame (utils/table_anchor.h), as the Quest's eyes are.

#include "gos_ar.h"

#include "gameos.hpp"
#include "utils/ar_view.h"
#include "utils/graphics.h"
#include "utils/table_anchor.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_system.h>
#include <jni.h>

#include <arcore_c_api.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace gos_ar {

namespace {

// ARCore's near and far planes: only the projection's angles are used
const float kNear = 0.05f;
const float kFar = 100.0f;
// how often the display's rotation is asked for (frames): the phone can turn
// over between the two landscape ways
const int kRotationEvery = 30;

struct State {
    ArSession* session = nullptr;
    ArFrame* frame = nullptr;
    bool running = false;       // resumed, not paused
    GLuint cameraTexture = 0;
    GLuint program = 0;
    GLuint vao = 0;
    GLint uvLocation = -1;
    float uvs[8] = {};          // the camera image's corners for the screen's
    bool haveUvs = false;

    int width = 0, height = 0, rotation = -1;
    int framesToRotation = 0;

    bool tracking = false;
    float position[3] = {};     // the phone's camera, as the display is turned
    float orientation[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float fov[4] = {};

    table_anchor::Settings tableSettings{};
    table_anchor::Table table{};
    bool tablePlaced = false;   // anywhere yet (ahead of the phone at first)
    ArAnchor* anchor = nullptr; // on a surface

    // the finger (drawable pixels) and the game's answer for it: each new
    // point is a new serial, a ray asked for carries it, the answer is for it
    bool haveFinger = false;
    float fingerX = 0.0f, fingerY = 0.0f;
    unsigned fingerSerial = 0, askedSerial = 0, answeredSerial = 0;
    int unanswered = 0;         // polls the game has not answered (no mission)
    bool worldValid = false;
    float worldX = 0.0f, worldY = 0.0f;     // 0 to 1 across the game's screen
    // the mouse as last answered: a drag's pointer stays there while a newer
    // point waits, instead of jumping to the finger's screen pixel
    bool haveMouse = false;
    float mouseX = 0.0f, mouseY = 0.0f;
};
State g;

JNIEnv* env()
{
    return (JNIEnv*)SDL_AndroidGetJNIEnv();
}

// the display's rotation (Surface.ROTATION_*), which ARCore needs to turn
// its camera image and projection the way the screen shows them
int displayRotation()
{
    JNIEnv* e = env();
    jobject activity = (jobject)SDL_AndroidGetActivity();
    if(!e || !activity)
        return 0;
    int rotation = 0;
    jclass activityClass = e->GetObjectClass(activity);
    jmethodID getWindowManager = e->GetMethodID(activityClass, "getWindowManager", "()Landroid/view/WindowManager;");
    jobject windowManager = getWindowManager ? e->CallObjectMethod(activity, getWindowManager) : nullptr;
    if(windowManager) {
        jclass managerClass = e->GetObjectClass(windowManager);
        jmethodID getDisplay = e->GetMethodID(managerClass, "getDefaultDisplay", "()Landroid/view/Display;");
        jobject display = getDisplay ? e->CallObjectMethod(windowManager, getDisplay) : nullptr;
        if(display) {
            jclass displayClass = e->GetObjectClass(display);
            jmethodID getRotation = e->GetMethodID(displayClass, "getRotation", "()I");
            if(getRotation)
                rotation = e->CallIntMethod(display, getRotation);
            e->DeleteLocalRef(displayClass);
            e->DeleteLocalRef(display);
        }
        e->DeleteLocalRef(managerClass);
        e->DeleteLocalRef(windowManager);
    }
    e->DeleteLocalRef(activityClass);
    e->DeleteLocalRef(activity);
    if(e->ExceptionCheck())
        e->ExceptionClear();
    return rotation;
}

GLuint compileShader(GLenum type, const char* source)
{
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if(!compiled) {
        char log[512] = {};
        glGetShaderInfoLog(shader, sizeof(log) - 1, nullptr, log);
        printf("[AR] camera shader: %s\n", log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

// The camera image over the whole window: a strip of two triangles with the
// image's corners for the screen's (ArFrame_transformCoordinates2d). GLES
// only, with an external texture: not a game shader (shader-conventions.md).
bool createBackground()
{
    const char* vertex =
        "#version 300 es\n"
        "uniform vec2 uvs[4];\n"
        "out vec2 uv;\n"
        "void main() {\n"
        "    vec2 p = vec2((gl_VertexID & 1) == 1 ? 1.0 : -1.0, (gl_VertexID & 2) == 2 ? 1.0 : -1.0);\n"
        "    uv = uvs[gl_VertexID];\n"
        "    gl_Position = vec4(p, 0.0, 1.0);\n"
        "}\n";
    const char* fragment =
        "#version 300 es\n"
        "#extension GL_OES_EGL_image_external_essl3 : require\n"
        "precision mediump float;\n"
        "uniform samplerExternalOES camera;\n"
        "in vec2 uv;\n"
        "out vec4 colour;\n"
        "void main() {\n"
        "    colour = vec4(texture(camera, uv).rgb, 1.0);\n"
        "}\n";
    const GLuint vs = compileShader(GL_VERTEX_SHADER, vertex);
    const GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragment);
    if(!vs || !fs) {
        if(vs) glDeleteShader(vs);
        if(fs) glDeleteShader(fs);
        return false;
    }
    g.program = glCreateProgram();
    glAttachShader(g.program, vs);
    glAttachShader(g.program, fs);
    glLinkProgram(g.program);
    glDeleteShader(vs);
    glDeleteShader(fs);
    GLint linked = 0;
    glGetProgramiv(g.program, GL_LINK_STATUS, &linked);
    if(!linked) {
        printf("[AR] camera shader did not link\n");
        glDeleteProgram(g.program);
        g.program = 0;
        return false;
    }
    g.uvLocation = glGetUniformLocation(g.program, "uvs");
    GLint previous = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &previous);
    glUseProgram(g.program);
    glUniform1i(glGetUniformLocation(g.program, "camera"), 0);
    glUseProgram((GLuint)previous);
    glGenVertexArrays(1, &g.vao);

    glGenTextures(1, &g.cameraTexture);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, g.cameraTexture);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
    return true;
}

// The camera image, behind everything. The renderer caches GL state, so what
// this changes is put back.
void drawBackground()
{
    if(!g.program || !g.haveUvs)
        return;
    GLint program = 0, vao = 0, activeTexture = GL_TEXTURE0, external = 0;
    GLboolean depthMask = GL_TRUE;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
    const GLboolean depthTest = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean blend = glIsEnabled(GL_BLEND);
    const GLboolean cull = glIsEnabled(GL_CULL_FACE);
    const GLboolean scissor = glIsEnabled(GL_SCISSOR_TEST);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_EXTERNAL_OES, &external);

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    glUseProgram(g.program);
    glUniform2fv(g.uvLocation, 4, g.uvs);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, g.cameraTexture);
    glBindVertexArray(g.vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindVertexArray((GLuint)vao);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, (GLuint)external);
    glActiveTexture((GLenum)activeTexture);
    glUseProgram((GLuint)program);
    glDepthMask(depthMask);
    if(depthTest) glEnable(GL_DEPTH_TEST);
    if(blend) glEnable(GL_BLEND);
    if(cull) glEnable(GL_CULL_FACE);
    if(scissor) glEnable(GL_SCISSOR_TEST);
}

// a pose's position and orientation (x, y, z, w)
void readPose(const ArPose* pose, float position[3], float orientation[4])
{
    float raw[7];   // qx, qy, qz, qw, tx, ty, tz
    ArPose_getPoseRaw(g.session, pose, raw);
    orientation[0] = raw[0]; orientation[1] = raw[1]; orientation[2] = raw[2]; orientation[3] = raw[3];
    position[0] = raw[4]; position[1] = raw[5]; position[2] = raw[6];
}

// the heading a camera with this orientation looks along, about +y (0: -z)
float headingOf(const float q[4])
{
    // the camera's -z turned by q
    const float fx = -2.0f * (q[0] * q[2] + q[3] * q[1]);
    const float fz = -(1.0f - 2.0f * (q[0] * q[0] + q[1] * q[1]));
    return std::atan2(-fx, -fz);
}

// The table on the first level surface found under the middle of the screen,
// facing the way the phone faces: an anchor, which ARCore keeps in place as it
// learns the room better.
void findSurface()
{
    ArHitResultList* hits = nullptr;
    ArHitResultList_create(g.session, &hits);
    ArFrame_hitTest(g.session, g.frame, 0.5f * (float)g.width, 0.5f * (float)g.height, hits);
    int32_t count = 0;
    ArHitResultList_getSize(g.session, hits, &count);
    ArHitResult* hit = nullptr;
    ArHitResult_create(g.session, &hit);
    for(int32_t i = 0; i < count && !g.anchor; ++i) {
        ArHitResultList_getItem(g.session, hits, i, hit);
        ArTrackable* trackable = nullptr;
        ArHitResult_acquireTrackable(g.session, hit, &trackable);
        ArTrackableType type = AR_TRACKABLE_NOT_VALID;
        ArTrackable_getType(g.session, trackable, &type);
        if(type == AR_TRACKABLE_PLANE) {
            ArPlaneType planeType = AR_PLANE_VERTICAL;
            ArPlane_getType(g.session, (ArPlane*)trackable, &planeType);
            ArPose* pose = nullptr;
            ArPose_create(g.session, nullptr, &pose);
            ArHitResult_getHitPose(g.session, hit, pose);
            int32_t inside = 0;
            ArPlane_isPoseInPolygon(g.session, (ArPlane*)trackable, pose, &inside);
            ArPose_destroy(pose);
            if(planeType == AR_PLANE_HORIZONTAL_UPWARD_FACING && inside &&
               ArHitResult_acquireNewAnchor(g.session, hit, &g.anchor) == AR_SUCCESS) {
                g.table.yaw = headingOf(g.orientation);
                ArPose* at = nullptr;
                ArPose_create(g.session, nullptr, &at);
                ArAnchor_getPose(g.session, g.anchor, at);
                float unused[4];
                readPose(at, g.table.position, unused);
                ArPose_destroy(at);
                const float dx = g.table.position[0] - g.position[0];
                const float dy = g.table.position[1] - g.position[1];
                const float dz = g.table.position[2] - g.position[2];
                printf("[AR] table placed on a surface %.2f m from the phone, %.2f m below it, at 1:%.0f\n",
                        std::sqrt(dx * dx + dy * dy + dz * dz), -dy, g.tableSettings.scale);
            }
        }
        ArTrackable_release(trackable);
    }
    ArHitResult_destroy(hit);
    ArHitResultList_destroy(hits);
}

} // namespace

bool init()
{
    const char* on = getenv("MC2_AR_TABLE");
    if(!on || on[0] != '1')
        return false;
    printf("[AR] tabletop (MC2_AR_TABLE=1): asking for the camera\n");
    if(!SDL_AndroidRequestPermission("android.permission.CAMERA")) {
        printf("[AR] no camera permission: playing flat\n");
        return false;
    }
    JNIEnv* e = env();
    jobject activity = (jobject)SDL_AndroidGetActivity();
    if(!e || !activity) {
        printf("[AR] no activity: playing flat\n");
        return false;
    }
    ArInstallStatus install = AR_INSTALL_STATUS_INSTALL_REQUESTED;
    const ArStatus installed = ArCoreApk_requestInstall(e, activity, 1, &install);
    if(installed != AR_SUCCESS || install != AR_INSTALL_STATUS_INSTALLED) {
        printf("[AR] ARCore (Google Play Services for AR) is not ready (%d, %d): playing flat; start again once it is installed\n",
                (int)installed, (int)install);
        e->DeleteLocalRef(activity);
        return false;
    }
    const ArStatus created = ArSession_create(e, activity, &g.session);
    e->DeleteLocalRef(activity);
    if(created != AR_SUCCESS || !g.session) {
        printf("[AR] no ARCore session (%d): playing flat\n", (int)created);
        g.session = nullptr;
        return false;
    }

    ArConfig* config = nullptr;
    ArConfig_create(g.session, &config);
    ArConfig_setPlaneFindingMode(g.session, config, AR_PLANE_FINDING_MODE_HORIZONTAL);
    // the newest image without waiting for one: the game draws at the
    // screen's rate, the camera gives 30 a second
    ArConfig_setUpdateMode(g.session, config, AR_UPDATE_MODE_LATEST_CAMERA_IMAGE);
    ArConfig_setFocusMode(g.session, config, AR_FOCUS_MODE_AUTO);
    const ArStatus configured = ArSession_configure(g.session, config);
    ArConfig_destroy(config);
    if(configured != AR_SUCCESS || !createBackground()) {
        printf("[AR] ARCore could not be set up (%d): playing flat\n", (int)configured);
        shutdown();
        return false;
    }
    ArSession_setCameraTextureName(g.session, g.cameraTexture);
    ArFrame_create(g.session, &g.frame);

    g.tableSettings = table_anchor::settingsFrom(getenv("MC2_TABLE_SCALE"), getenv("MC2_TABLE_PLACE"));
    resume();
    printf("[AR] tabletop at 1:%.0f: the table goes on the first level surface in the middle of the screen\n",
            g.tableSettings.scale);
    return g.running;
}

bool active()
{
    return g.session != nullptr;
}

void beginFrame()
{
    if(!g.running)
        return;
    // the window's size and the display's rotation, for the camera image's
    // corners and the projection
    if(--g.framesToRotation <= 0 || g.width != Environment.drawableWidth || g.height != Environment.drawableHeight) {
        g.framesToRotation = kRotationEvery;
        const int rotation = displayRotation();
        if(rotation != g.rotation || g.width != Environment.drawableWidth || g.height != Environment.drawableHeight) {
            g.rotation = rotation;
            g.width = Environment.drawableWidth;
            g.height = Environment.drawableHeight;
            ArSession_setDisplayGeometry(g.session, g.rotation, g.width, g.height);
        }
    }
    if(ArSession_update(g.session, g.frame) != AR_SUCCESS) {
        g.tracking = false;
        return;
    }
    int32_t changed = 0;
    ArFrame_getDisplayGeometryChanged(g.session, g.frame, &changed);
    if(changed || !g.haveUvs) {
        // the screen's corners in the order the strip draws them
        const float corners[8] = {-1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f};
        ArFrame_transformCoordinates2d(g.session, g.frame, AR_COORDINATES_2D_OPENGL_NORMALIZED_DEVICE_COORDINATES,
                4, corners, AR_COORDINATES_2D_TEXTURE_NORMALIZED, g.uvs);
        g.haveUvs = true;
    }

    ArCamera* camera = nullptr;
    ArFrame_acquireCamera(g.session, g.frame, &camera);
    ArTrackingState state = AR_TRACKING_STATE_STOPPED;
    ArCamera_getTrackingState(g.session, camera, &state);
    const bool wasTracking = g.tracking;
    g.tracking = state == AR_TRACKING_STATE_TRACKING;
    if(g.tracking) {
        ArPose* pose = nullptr;
        ArPose_create(g.session, nullptr, &pose);
        ArCamera_getDisplayOrientedPose(g.session, camera, pose);
        readPose(pose, g.position, g.orientation);
        ArPose_destroy(pose);
        float projection[16];
        ArCamera_getProjectionMatrix(g.session, camera, kNear, kFar, projection);
        ar_view::fovFromProjection(projection, g.fov);
    }
    ArCamera_release(camera);
    if(g.tracking != wasTracking)
        printf("[AR] %s\n", g.tracking ? "tracking" : "tracking lost: move the phone slowly over a textured surface");
    if(!g.tracking)
        return;

    // the table: ahead of and below the phone until a surface is found
    if(!g.tablePlaced) {
        g.table = table_anchor::inFront(g.position, headingOf(g.orientation),
                g.tableSettings.distance, g.tableSettings.drop);
        g.tablePlaced = true;
    }
    if(!g.anchor) {
        findSurface();
    } else {
        ArTrackingState anchorState = AR_TRACKING_STATE_STOPPED;
        ArAnchor_getTrackingState(g.session, g.anchor, &anchorState);
        if(anchorState == AR_TRACKING_STATE_TRACKING) {
            ArPose* at = nullptr;
            ArPose_create(g.session, nullptr, &at);
            ArAnchor_getPose(g.session, g.anchor, at);
            float unused[4];
            readPose(at, g.table.position, unused);
            ArPose_destroy(at);
        }
    }
}

int eyeViews(gosVREyeView views[2])
{
    if(!g.running || !g.tracking)
        return 0;
    table_anchor::toTable(g.table, g.position, g.orientation, views[0].position, views[0].orientation);
    memcpy(views[0].fov, g.fov, sizeof(g.fov));
    return 1;
}

void eyeImageSize(int, int& width, int& height)
{
    width = Environment.drawableWidth;
    height = Environment.drawableHeight;
}

bool bindEye(int eye)
{
    if(!g.running || eye != 0)
        return false;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, Environment.drawableWidth, Environment.drawableHeight);
    glDepthMask(GL_TRUE);
    glClear(GL_DEPTH_BUFFER_BIT);
    drawBackground();
    return true;
}

float tableScale()
{
    return g.tableSettings.scale;
}

bool pointerFor(float x, float y, float& mouseX, float& mouseY)
{
    mouseX = x;
    mouseY = y;
    // not tracking: the game draws its own screen, where the finger is right
    if(!g.running || !g.tracking) {
        g.haveMouse = false;
        return true;
    }
    if(!g.haveFinger || x != g.fingerX || y != g.fingerY) {
        g.haveFinger = true;
        g.fingerX = x;
        g.fingerY = y;
        ++g.fingerSerial;
        g.unanswered = 0;
    }
    if(g.answeredSerial != g.fingerSerial) {
        // not answered yet: the pointer stays where it was (a drag lags a
        // frame rather than jumping), and waits, but not for ever (no
        // mission picks)
        if(g.haveMouse) {
            mouseX = g.mouseX;
            mouseY = g.mouseY;
        }
        return ++g.unanswered > 3;
    }
    if(g.worldValid) {
        int vp[4];
        gos_GetWindowViewport(vp);
        const int top = Environment.drawableHeight - vp[1] - vp[3];
        mouseX = (float)vp[0] + g.worldX * (float)vp[2];
        mouseY = (float)top + g.worldY * (float)vp[3];
    }
    g.haveMouse = true;
    g.mouseX = mouseX;
    g.mouseY = mouseY;
    return true;
}

bool touchPoint(float& x, float& y)
{
    if(!g.running || !g.haveFinger)
        return false;
    int vp[4];
    gos_GetWindowViewport(vp);
    if(vp[2] <= 0 || vp[3] <= 0)
        return false;
    const int top = Environment.drawableHeight - vp[1] - vp[3];
    x = (g.fingerX - (float)vp[0]) / (float)vp[2];
    y = (g.fingerY - (float)top) / (float)vp[3];
    return true;
}

bool pointerRay(float position[3], float orientation[4])
{
    if(!g.running || !g.tracking || !g.haveFinger || Environment.drawableWidth <= 0 || Environment.drawableHeight <= 0)
        return false;
    // the camera image fills the window, as the projection does
    float ray[4];
    ar_view::rayThroughPixel(g.orientation, g.fov, g.fingerX / (float)Environment.drawableWidth,
            g.fingerY / (float)Environment.drawableHeight, ray);
    table_anchor::toTable(g.table, g.position, ray, position, orientation);
    g.askedSerial = g.fingerSerial;
    return true;
}

void setWorldPointer(bool valid, float x, float y)
{
    if(!g.running)
        return;
    g.answeredSerial = g.askedSerial;
    g.worldValid = valid;
    g.worldX = x;
    g.worldY = y;
}

void pause()
{
    if(g.session && g.running) {
        ArSession_pause(g.session);
        g.running = false;
        g.tracking = false;
    }
}

void resume()
{
    if(g.session && !g.running) {
        const ArStatus resumed = ArSession_resume(g.session);
        g.running = resumed == AR_SUCCESS;
        if(!g.running)
            printf("[AR] ARCore did not resume (%d)\n", (int)resumed);
    }
}

void shutdown()
{
    if(g.anchor) {
        ArAnchor_release(g.anchor);
        g.anchor = nullptr;
    }
    if(g.frame) {
        ArFrame_destroy(g.frame);
        g.frame = nullptr;
    }
    if(g.session) {
        ArSession_destroy(g.session);
        g.session = nullptr;
    }
    g.running = false;
    if(g.program) {
        glDeleteProgram(g.program);
        g.program = 0;
    }
    if(g.vao) {
        glDeleteVertexArrays(1, &g.vao);
        g.vao = 0;
    }
    if(g.cameraTexture) {
        glDeleteTextures(1, &g.cameraTexture);
        g.cameraTexture = 0;
    }
}

} // namespace gos_ar
