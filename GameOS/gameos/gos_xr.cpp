// OpenXR presentation for the Meta Quest build: see gos_xr.h.
//
// The game draws as before into the default framebuffer (SDL's window
// surface): the whole frame, interface included. In VR the camera also
// draws each eye into its own swapchain image (bindEye) during that frame.
// present() copies the window's frame into the quad's swapchain and submits
// the eyes as a projection layer with the quad on top, in LOCAL space
// (origin where the head was when the session started).

#include "gos_xr.h"

#include "gameos.hpp"
#include "utils/graphics.h"
#include "utils/gl_utils.h"
#include "utils/tga_writer.h"

#include <EGL/egl.h>
#include <SDL2/SDL.h>
#include <jni.h>

#define XR_USE_PLATFORM_ANDROID
#define XR_USE_GRAPHICS_API_OPENGL_ES
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace gos_xr {

namespace {

// the quad: the game frame on a big screen ahead without eyes; with them the
// interface alone (the window has no world then), see-through, where the
// player looks down at the battle, facing them
const float kScreenDistance = 1.5f;
const float kScreenWidth = 2.0f;
const float kHudDistance = 1.5f;
const float kHudDown = 0.52f;           // radians below the horizon (30 degrees)
const float kHudWidth = 2.0f;

struct Image {
    XrSwapchain swapchain = XR_NULL_HANDLE;
    int32_t width = 0, height = 0;
    std::vector<XrSwapchainImageOpenGLESKHR> images;
    GLuint fbo = 0;
    GLuint depth = 0;           // eyes only
    bool acquired = false;
};

struct State {
    XrInstance instance = XR_NULL_HANDLE;
    XrSystemId system = XR_NULL_SYSTEM_ID;
    XrSession session = XR_NULL_HANDLE;
    XrSpace space = XR_NULL_HANDLE;
    Image quad;
    Image eyes[2];
    bool useEyes = false;
    // the eyes' multisampling (MC2_XR_MSAA, default 4; 1 = off), rendered
    // to texture and resolved by the tiler (GL_EXT_multisampled_render_to_texture)
    int samples = 1;
    PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC framebufferTexture2DMultisample = nullptr;
    PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC renderbufferStorageMultisample = nullptr;
    bool running = false;
    bool exit = false;
    // this frame
    bool frameBegun = false;
    XrFrameState frame = {};
    XrView views[2] = {};
    bool viewsValid = false;
    // debugging (MC2_XR_POSE, MC2_XR_SHOT)
    bool fixedPose = false;
    XrQuaternionf fixedOrientation = {};
    int shotFrame = -1;
    int eyeFrames = 0;
};
State g;

// an OpenXR input or output struct: zeroed, with its type set
template<typename T> T xrStruct(XrStructureType type)
{
    T s;
    memset(&s, 0, sizeof(s));
    s.type = type;
    return s;
}

bool ok(XrResult r, const char* what)
{
    if(XR_SUCCEEDED(r))
        return true;
    char text[XR_MAX_RESULT_STRING_SIZE] = "";
    if(g.instance != XR_NULL_HANDLE)
        xrResultToString(g.instance, r, text);
    printf("[XR] %s failed: %d %s\n", what, (int)r, text);
    return false;
}

bool initLoader(JavaVM* vm, jobject activity)
{
    PFN_xrInitializeLoaderKHR initializeLoader = nullptr;
    if(!ok(xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR",
                (PFN_xrVoidFunction*)&initializeLoader), "xrGetInstanceProcAddr(xrInitializeLoaderKHR)"))
        return false;
    XrLoaderInitInfoAndroidKHR info = xrStruct<XrLoaderInitInfoAndroidKHR>(XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR);
    info.applicationVM = vm;
    info.applicationContext = activity;
    return ok(initializeLoader((const XrLoaderInitInfoBaseHeaderKHR*)&info), "xrInitializeLoaderKHR");
}

bool createInstance(JavaVM* vm, jobject activity)
{
    const char* extensions[] = {
        XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME,
        XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME,
    };
    XrInstanceCreateInfoAndroidKHR android = xrStruct<XrInstanceCreateInfoAndroidKHR>(XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR);
    android.applicationVM = vm;
    android.applicationActivity = activity;

    XrInstanceCreateInfo info = xrStruct<XrInstanceCreateInfo>(XR_TYPE_INSTANCE_CREATE_INFO);
    info.next = &android;
    strncpy(info.applicationInfo.applicationName, "MechCommander 2", XR_MAX_APPLICATION_NAME_SIZE - 1);
    info.applicationInfo.applicationVersion = 1;
    strncpy(info.applicationInfo.engineName, "MC2 GameOS", XR_MAX_ENGINE_NAME_SIZE - 1);
    info.applicationInfo.apiVersion = XR_API_VERSION_1_0;
    info.enabledExtensionCount = sizeof(extensions) / sizeof(extensions[0]);
    info.enabledExtensionNames = extensions;
    if(!ok(xrCreateInstance(&info, &g.instance), "xrCreateInstance"))
        return false;

    XrInstanceProperties props = xrStruct<XrInstanceProperties>(XR_TYPE_INSTANCE_PROPERTIES);
    if(XR_SUCCEEDED(xrGetInstanceProperties(g.instance, &props)))
        printf("[XR] runtime: %s %u.%u.%u\n", props.runtimeName,
                XR_VERSION_MAJOR(props.runtimeVersion), XR_VERSION_MINOR(props.runtimeVersion),
                XR_VERSION_PATCH(props.runtimeVersion));
    return true;
}

bool createSession()
{
    XrSystemGetInfo systemInfo = xrStruct<XrSystemGetInfo>(XR_TYPE_SYSTEM_GET_INFO);
    systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    if(!ok(xrGetSystem(g.instance, &systemInfo, &g.system), "xrGetSystem"))
        return false;

    // required before xrCreateSession
    PFN_xrGetOpenGLESGraphicsRequirementsKHR getRequirements = nullptr;
    if(!ok(xrGetInstanceProcAddr(g.instance, "xrGetOpenGLESGraphicsRequirementsKHR",
                (PFN_xrVoidFunction*)&getRequirements), "xrGetInstanceProcAddr(GLES requirements)"))
        return false;
    XrGraphicsRequirementsOpenGLESKHR requirements = xrStruct<XrGraphicsRequirementsOpenGLESKHR>(XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR);
    if(!ok(getRequirements(g.instance, g.system, &requirements), "xrGetOpenGLESGraphicsRequirementsKHR"))
        return false;

    // SDL's context: the binding needs its display, config and context
    XrGraphicsBindingOpenGLESAndroidKHR binding = xrStruct<XrGraphicsBindingOpenGLESAndroidKHR>(XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR);
    binding.display = eglGetCurrentDisplay();
    binding.context = eglGetCurrentContext();
    EGLint configId = 0;
    eglQueryContext(binding.display, binding.context, EGL_CONFIG_ID, &configId);
    const EGLint attribs[] = {EGL_CONFIG_ID, configId, EGL_NONE};
    EGLint count = 0;
    if(!eglChooseConfig(binding.display, attribs, &binding.config, 1, &count) || count != 1) {
        printf("[XR] no EGL config for the current context\n");
        return false;
    }

    XrSessionCreateInfo info = xrStruct<XrSessionCreateInfo>(XR_TYPE_SESSION_CREATE_INFO);
    info.next = &binding;
    info.systemId = g.system;
    if(!ok(xrCreateSession(g.instance, &info, &g.session), "xrCreateSession"))
        return false;

    XrReferenceSpaceCreateInfo spaceInfo = xrStruct<XrReferenceSpaceCreateInfo>(XR_TYPE_REFERENCE_SPACE_CREATE_INFO);
    spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    spaceInfo.poseInReferenceSpace.orientation.w = 1.0f;
    return ok(xrCreateReferenceSpace(g.session, &spaceInfo, &g.space), "xrCreateReferenceSpace");
}

bool createImage(Image& image, int width, int height, bool withDepth, const char* name)
{
    uint32_t count = 0;
    xrEnumerateSwapchainFormats(g.session, 0, &count, nullptr);
    std::vector<int64_t> formats(count);
    xrEnumerateSwapchainFormats(g.session, count, &count, formats.data());
    // the compositor reads an sRGB image as gamma-encoded, which is what the
    // game's colours are; writes to it are kept unconverted (init())
    int64_t format = 0;
    for(int64_t f : formats)
        if(f == GL_SRGB8_ALPHA8)
            format = f;
    if(!format) {
        printf("[XR] no GL_SRGB8_ALPHA8 swapchain format\n");
        return false;
    }

    XrSwapchainCreateInfo info = xrStruct<XrSwapchainCreateInfo>(XR_TYPE_SWAPCHAIN_CREATE_INFO);
    info.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
    info.format = format;
    info.sampleCount = 1;
    info.width = (uint32_t)width;
    info.height = (uint32_t)height;
    info.faceCount = 1;
    info.arraySize = 1;
    info.mipCount = 1;
    if(!ok(xrCreateSwapchain(g.session, &info, &image.swapchain), "xrCreateSwapchain"))
        return false;
    image.width = width;
    image.height = height;

    xrEnumerateSwapchainImages(image.swapchain, 0, &count, nullptr);
    image.images.assign(count, xrStruct<XrSwapchainImageOpenGLESKHR>(XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR));
    if(!ok(xrEnumerateSwapchainImages(image.swapchain, count, &count,
                (XrSwapchainImageBaseHeader*)image.images.data()), "xrEnumerateSwapchainImages"))
        return false;
    glGenFramebuffers(1, &image.fbo);
    if(withDepth) {
        glGenRenderbuffers(1, &image.depth);
        glBindRenderbuffer(GL_RENDERBUFFER, image.depth);
        if(g.samples > 1)
            g.renderbufferStorageMultisample(GL_RENDERBUFFER, g.samples, GL_DEPTH_COMPONENT24, width, height);
        else
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }
    printf("[XR] %s: %dx%d, %u images\n", name, width, height, count);
    return true;
}

bool createEyes()
{
    uint32_t count = 0;
    xrEnumerateViewConfigurationViews(g.instance, g.system, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &count, nullptr);
    std::vector<XrViewConfigurationView> configs(count, xrStruct<XrViewConfigurationView>(XR_TYPE_VIEW_CONFIGURATION_VIEW));
    if(!ok(xrEnumerateViewConfigurationViews(g.instance, g.system, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                count, &count, configs.data()), "xrEnumerateViewConfigurationViews") || count != 2)
        return false;
    for(int eye = 0; eye < 2; ++eye)
        if(!createImage(g.eyes[eye], (int)configs[eye].recommendedImageRectWidth,
                    (int)configs[eye].recommendedImageRectHeight, true, eye ? "right eye" : "left eye"))
            return false;
    return true;
}

void pollEvents()
{
    XrEventDataBuffer event = xrStruct<XrEventDataBuffer>(XR_TYPE_EVENT_DATA_BUFFER);
    while(xrPollEvent(g.instance, &event) == XR_SUCCESS) {
        if(event.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
            const XrEventDataSessionStateChanged& changed = *(XrEventDataSessionStateChanged*)&event;
            printf("[XR] session state %d\n", (int)changed.state);
            if(changed.state == XR_SESSION_STATE_READY) {
                XrSessionBeginInfo begin = xrStruct<XrSessionBeginInfo>(XR_TYPE_SESSION_BEGIN_INFO);
                begin.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
                g.running = ok(xrBeginSession(g.session, &begin), "xrBeginSession");
            } else if(changed.state == XR_SESSION_STATE_STOPPING) {
                xrEndSession(g.session);
                g.running = false;
            } else if(changed.state == XR_SESSION_STATE_EXITING || changed.state == XR_SESSION_STATE_LOSS_PENDING) {
                g.exit = true;
            }
        } else if(event.type == XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING) {
            g.exit = true;
        }
        event = xrStruct<XrEventDataBuffer>(XR_TYPE_EVENT_DATA_BUFFER);
    }
}

// Acquires the image's next texture and attaches it to the image's FBO.
bool acquire(Image& image)
{
    uint32_t index = 0;
    if(!ok(xrAcquireSwapchainImage(image.swapchain, nullptr, &index), "xrAcquireSwapchainImage"))
        return false;
    XrSwapchainImageWaitInfo wait = xrStruct<XrSwapchainImageWaitInfo>(XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO);
    wait.timeout = XR_INFINITE_DURATION;
    if(!ok(xrWaitSwapchainImage(image.swapchain, &wait), "xrWaitSwapchainImage")) {
        XrSwapchainImageReleaseInfo release = xrStruct<XrSwapchainImageReleaseInfo>(XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO);
        xrReleaseSwapchainImage(image.swapchain, &release);
        return false;
    }
    image.acquired = true;
    glBindFramebuffer(GL_FRAMEBUFFER, image.fbo);
    if(image.depth && g.samples > 1)
        g.framebufferTexture2DMultisample(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                image.images[index].image, 0, g.samples);
    else
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, image.images[index].image, 0);
    if(image.depth)
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, image.depth);
    return true;
}

void release(Image& image)
{
    if(!image.acquired)
        return;
    XrSwapchainImageReleaseInfo info = xrStruct<XrSwapchainImageReleaseInfo>(XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO);
    ok(xrReleaseSwapchainImage(image.swapchain, &info), "xrReleaseSwapchainImage");
    image.acquired = false;
}

// Copies the default framebuffer into the quad's next image.
bool copyFrame(int width, int height)
{
    if(!acquire(g.quad))
        return false;
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, width, height, 0, 0, g.quad.width, g.quad.height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    release(g.quad);
    return true;
}

// MC2_XR_POSE=yaw,pitch (degrees): a fixed head rotation instead of the
// tracked one, for checking directions with the headset on a desk
void readDebugSettings()
{
    const char* pose = getenv("MC2_XR_POSE");
    float yaw = 0.0f, pitch = 0.0f;
    if(pose && sscanf(pose, "%f,%f", &yaw, &pitch) == 2) {
        const float y = yaw * 3.14159265f / 360.0f, p = pitch * 3.14159265f / 360.0f;
        // yaw about +y (left), then pitch about +x (up), in OpenXR's axes
        g.fixedOrientation.x = std::cos(y) * std::sin(p);
        g.fixedOrientation.y = std::sin(y) * std::cos(p);
        g.fixedOrientation.z = -std::sin(y) * std::sin(p);
        g.fixedOrientation.w = std::cos(y) * std::cos(p);
        g.fixedPose = true;
        printf("[XR] fixed head pose: yaw %.0f, pitch %.0f degrees (MC2_XR_POSE)\n", yaw, pitch);
    }
    // MC2_XR_SHOT=n: the left eye's image and the window frame as TGAs on
    // the n-th frame with eyes drawn
    const char* shot = getenv("MC2_XR_SHOT");
    if(shot)
        g.shotFrame = atoi(shot);
}

void saveImage(GLuint fbo, int width, int height, const char* path)
{
    std::vector<uint8_t> pixels((size_t)width * height * 4);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    printf("[XR] %s %s\n", writeTGA24(path, width, height, pixels.data()) ? "saved" : "could not save", path);
}

XrPosef quadPose()
{
    XrPosef pose = {};
    if(g.useEyes) {
        // turned about x to face the eyes
        pose.orientation.x = -std::sin(0.5f * kHudDown);
        pose.orientation.w = std::cos(0.5f * kHudDown);
        pose.position.y = -kHudDistance * std::sin(kHudDown);
        pose.position.z = -kHudDistance * std::cos(kHudDown);
    } else {
        pose.orientation.w = 1.0f;
        pose.position.z = -kScreenDistance;
    }
    return pose;
}

} // namespace

bool init()
{
    const char* env = getenv("MC2_XR");
    if(env && env[0] == '0') {
        printf("[XR] off (MC2_XR=0)\n");
        return false;
    }
    JNIEnv* jni = (JNIEnv*)SDL_AndroidGetJNIEnv();
    jobject activity = (jobject)SDL_AndroidGetActivity();
    JavaVM* vm = nullptr;
    if(!jni || !activity || jni->GetJavaVM(&vm) != JNI_OK) {
        printf("[XR] no Java VM or activity\n");
        return false;
    }
    // the loader and the runtime keep the activity: a global reference
    jobject activityRef = jni->NewGlobalRef(activity);
    jni->DeleteLocalRef(activity);

    readDebugSettings();
    const char* eyesEnv = getenv("MC2_XR_EYES");
    g.useEyes = !(eyesEnv && eyesEnv[0] == '0');
    const char* msaaEnv = getenv("MC2_XR_MSAA");
    const int samples = msaaEnv ? atoi(msaaEnv) : 4;
    g.framebufferTexture2DMultisample = (PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC)
            eglGetProcAddress("glFramebufferTexture2DMultisampleEXT");
    g.renderbufferStorageMultisample = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC)
            eglGetProcAddress("glRenderbufferStorageMultisampleEXT");
    if(samples > 1 && g.framebufferTexture2DMultisample && g.renderbufferStorageMultisample)
        g.samples = samples;
    printf("[XR] eyes: %dx MSAA\n", g.samples);

    int width = 0, height = 0;
    SDL_GL_GetDrawableSize(SDL_GL_GetCurrentWindow(), &width, &height);
    if(initLoader(vm, activityRef) && createInstance(vm, activityRef) && createSession()
            && createImage(g.quad, width, height, false, "screen") && (!g.useEyes || createEyes())) {
        // textures loaded from now on get mipmaps: distant units shimmer in
        // the headset without them
        setGenerateMipmaps(true);
#ifdef GL_FRAMEBUFFER_SRGB_EXT
        // the game's colours are already gamma-encoded: store them as they
        // are in the sRGB swapchain images
        glDisable(GL_FRAMEBUFFER_SRGB_EXT);
#endif
        printf("[XR] on: %s\n", g.useEyes ? "head-tracked eyes, the interface as a HUD below"
                : "the game frame on a screen ahead (MC2_XR_EYES=0)");
        return true;
    }
    shutdown();
    return false;
}

void beginFrame()
{
    g.frameBegun = false;
    g.viewsValid = false;
    if(g.session == XR_NULL_HANDLE)
        return;
    pollEvents();
    if(!g.running)
        return;

    XrFrameWaitInfo waitInfo = xrStruct<XrFrameWaitInfo>(XR_TYPE_FRAME_WAIT_INFO);
    g.frame = xrStruct<XrFrameState>(XR_TYPE_FRAME_STATE);
    if(!ok(xrWaitFrame(g.session, &waitInfo, &g.frame), "xrWaitFrame"))
        return;
    XrFrameBeginInfo beginInfo = xrStruct<XrFrameBeginInfo>(XR_TYPE_FRAME_BEGIN_INFO);
    if(!ok(xrBeginFrame(g.session, &beginInfo), "xrBeginFrame"))
        return;
    g.frameBegun = true;

    if(!g.useEyes || !g.frame.shouldRender)
        return;
    XrViewLocateInfo locate = xrStruct<XrViewLocateInfo>(XR_TYPE_VIEW_LOCATE_INFO);
    locate.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    locate.displayTime = g.frame.predictedDisplayTime;
    locate.space = g.space;
    XrViewState state = xrStruct<XrViewState>(XR_TYPE_VIEW_STATE);
    uint32_t count = 0;
    g.views[0] = g.views[1] = xrStruct<XrView>(XR_TYPE_VIEW);
    if(!ok(xrLocateViews(g.session, &locate, &state, 2, &count, g.views), "xrLocateViews") || count != 2)
        return;
    const XrViewStateFlags tracked = XR_VIEW_STATE_ORIENTATION_VALID_BIT | XR_VIEW_STATE_POSITION_VALID_BIT;
    g.viewsValid = (state.viewStateFlags & tracked) == tracked;
    if(g.viewsValid && g.fixedPose) {
        // keep the eyes' spacing, centred on the start pose
        XrVector3f mid = {(g.views[0].pose.position.x + g.views[1].pose.position.x) * 0.5f,
                          (g.views[0].pose.position.y + g.views[1].pose.position.y) * 0.5f,
                          (g.views[0].pose.position.z + g.views[1].pose.position.z) * 0.5f};
        for(int eye = 0; eye < 2; ++eye) {
            g.views[eye].pose.orientation = g.fixedOrientation;
            g.views[eye].pose.position.x -= mid.x;
            g.views[eye].pose.position.y -= mid.y;
            g.views[eye].pose.position.z -= mid.z;
        }
    }
}

int eyeViews(gosVREyeView views[2])
{
    if(!g.viewsValid)
        return 0;
    for(int eye = 0; eye < 2; ++eye) {
        const XrPosef& pose = g.views[eye].pose;
        views[eye].position[0] = pose.position.x;
        views[eye].position[1] = pose.position.y;
        views[eye].position[2] = pose.position.z;
        views[eye].orientation[0] = pose.orientation.x;
        views[eye].orientation[1] = pose.orientation.y;
        views[eye].orientation[2] = pose.orientation.z;
        views[eye].orientation[3] = pose.orientation.w;
        views[eye].fov[0] = g.views[eye].fov.angleLeft;
        views[eye].fov[1] = g.views[eye].fov.angleRight;
        views[eye].fov[2] = g.views[eye].fov.angleUp;
        views[eye].fov[3] = g.views[eye].fov.angleDown;
    }
    return 2;
}

bool bindEye(int eye)
{
    if(g.session == XR_NULL_HANDLE)
        return false;
    if(eye < 0 || eye > 1 || !g.viewsValid) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }
    Image& image = g.eyes[eye];
    if(!image.acquired && !acquire(image)) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, image.fbo);
    glViewport(0, 0, image.width, image.height);
    glDepthMask(GL_TRUE);
    // transparent black: the clear colour is global state, and the window's
    // clear (draw_screen) must leave the HUD's background see-through
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    return true;
}

bool present(int width, int height)
{
    if(g.session == XR_NULL_HANDLE)
        return false;
    if(!g.frameBegun) {
        // not visible yet: no frame paces the loop
        SDL_Delay(10);
        return true;
    }
    g.frameBegun = false;

    const bool eyesDrawn = g.eyes[0].acquired && g.eyes[1].acquired;
    if(eyesDrawn && g.eyeFrames++ == g.shotFrame) {
        saveImage(g.eyes[0].fbo, g.eyes[0].width, g.eyes[0].height, "xr_left_eye.tga");
        saveImage(0, width, height, "xr_window.tga");
    }
    release(g.eyes[0]);
    release(g.eyes[1]);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    XrCompositionLayerProjectionView projViews[2];
    XrCompositionLayerProjection projection = xrStruct<XrCompositionLayerProjection>(XR_TYPE_COMPOSITION_LAYER_PROJECTION);
    XrCompositionLayerQuad quad = xrStruct<XrCompositionLayerQuad>(XR_TYPE_COMPOSITION_LAYER_QUAD);
    const XrCompositionLayerBaseHeader* layers[2];
    uint32_t layerCount = 0;

    if(g.frame.shouldRender && eyesDrawn && g.viewsValid) {
        for(int eye = 0; eye < 2; ++eye) {
            projViews[eye] = xrStruct<XrCompositionLayerProjectionView>(XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW);
            projViews[eye].pose = g.views[eye].pose;
            projViews[eye].fov = g.views[eye].fov;
            projViews[eye].subImage.swapchain = g.eyes[eye].swapchain;
            projViews[eye].subImage.imageRect.extent = {g.eyes[eye].width, g.eyes[eye].height};
        }
        projection.space = g.space;
        projection.viewCount = 2;
        projection.views = projViews;
        layers[layerCount++] = (XrCompositionLayerBaseHeader*)&projection;
    }
    if(g.frame.shouldRender && copyFrame(width, height)) {
        quad.space = g.space;
        quad.eyeVisibility = XR_EYE_VISIBILITY_BOTH;
        quad.subImage.swapchain = g.quad.swapchain;
        quad.subImage.imageRect.extent = {g.quad.width, g.quad.height};
        quad.pose = quadPose();
        if(g.useEyes)       // the interface over the eyes: blended by its alpha
            quad.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT
                    | XR_COMPOSITION_LAYER_UNPREMULTIPLIED_ALPHA_BIT;
        // the game's screen shape: it is stretched over the whole window
        const float quadWidth = g.useEyes ? kHudWidth : kScreenWidth;
        quad.size.width = quadWidth;
        quad.size.height = quadWidth * (float)Environment.screenHeight / (float)Environment.screenWidth;
        layers[layerCount++] = (XrCompositionLayerBaseHeader*)&quad;
    }

    XrFrameEndInfo endInfo = xrStruct<XrFrameEndInfo>(XR_TYPE_FRAME_END_INFO);
    endInfo.displayTime = g.frame.predictedDisplayTime;
    endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    endInfo.layerCount = layerCount;
    endInfo.layers = layers;
    ok(xrEndFrame(g.session, &endInfo), "xrEndFrame");
    return true;
}

bool exitRequested()
{
    return g.exit;
}

bool active()
{
    return g.session != XR_NULL_HANDLE;
}

double logicStep()
{
    if(g.session == XR_NULL_HANDLE)
        return 0.0;
    static const int hz = getenv("MC2_XR_LOGIC_HZ") ? atoi(getenv("MC2_XR_LOGIC_HZ")) : 30;
    return hz > 0 ? 1.0 / hz : 0.0;
}


void shutdown()
{
    Image* images[] = {&g.quad, &g.eyes[0], &g.eyes[1]};
    for(Image* image : images) {
        if(image->fbo)
            glDeleteFramebuffers(1, &image->fbo);
        if(image->depth)
            glDeleteRenderbuffers(1, &image->depth);
        if(image->swapchain != XR_NULL_HANDLE)
            xrDestroySwapchain(image->swapchain);
    }
    if(g.space != XR_NULL_HANDLE)
        xrDestroySpace(g.space);
    if(g.session != XR_NULL_HANDLE)
        xrDestroySession(g.session);
    if(g.instance != XR_NULL_HANDLE)
        xrDestroyInstance(g.instance);
    g = State();
}

} // namespace gos_xr
