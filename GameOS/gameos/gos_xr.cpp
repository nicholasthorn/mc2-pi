// OpenXR presentation for the Meta Quest build: see gos_xr.h.
//
// The game renders as before into the default framebuffer (SDL's window
// surface). present() copies that frame into an OpenXR swapchain image and
// submits it as a quad layer in LOCAL space, 1.5 m in front of where the head
// was when the session started, so the player looks around a fixed screen.

#include "gos_xr.h"

#include "utils/graphics.h"

#include <EGL/egl.h>
#include <SDL2/SDL.h>
#include <jni.h>

#define XR_USE_PLATFORM_ANDROID
#define XR_USE_GRAPHICS_API_OPENGL_ES
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace gos_xr {

namespace {

const float kScreenDistance = 1.5f;     // metres in front of the start pose
const float kScreenWidth = 2.0f;        // metres

struct State {
    XrInstance instance = XR_NULL_HANDLE;
    XrSystemId system = XR_NULL_SYSTEM_ID;
    XrSession session = XR_NULL_HANDLE;
    XrSpace space = XR_NULL_HANDLE;
    XrSwapchain swapchain = XR_NULL_HANDLE;
    int32_t width = 0, height = 0;
    std::vector<XrSwapchainImageOpenGLESKHR> images;
    GLuint fbo = 0;
    bool running = false;
    bool exit = false;
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

bool createSwapchain(int width, int height)
{
    uint32_t count = 0;
    xrEnumerateSwapchainFormats(g.session, 0, &count, nullptr);
    std::vector<int64_t> formats(count);
    xrEnumerateSwapchainFormats(g.session, count, &count, formats.data());
    // the compositor reads an sRGB image as gamma-encoded, which is what the
    // game's colours are; writes to it are kept unconverted (see present())
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
    if(!ok(xrCreateSwapchain(g.session, &info, &g.swapchain), "xrCreateSwapchain"))
        return false;
    g.width = width;
    g.height = height;

    xrEnumerateSwapchainImages(g.swapchain, 0, &count, nullptr);
    g.images.assign(count, xrStruct<XrSwapchainImageOpenGLESKHR>(XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR));
    if(!ok(xrEnumerateSwapchainImages(g.swapchain, count, &count,
                (XrSwapchainImageBaseHeader*)g.images.data()), "xrEnumerateSwapchainImages"))
        return false;
    glGenFramebuffers(1, &g.fbo);
    printf("[XR] swapchain %dx%d, %u images\n", width, height, count);
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

// Copies the default framebuffer into the next swapchain image.
bool copyFrame(int width, int height)
{
    uint32_t index = 0;
    if(!ok(xrAcquireSwapchainImage(g.swapchain, nullptr, &index), "xrAcquireSwapchainImage"))
        return false;
    XrSwapchainImageWaitInfo wait = xrStruct<XrSwapchainImageWaitInfo>(XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO);
    wait.timeout = XR_INFINITE_DURATION;
    if(!ok(xrWaitSwapchainImage(g.swapchain, &wait), "xrWaitSwapchainImage"))
        return false;

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g.fbo);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
            g.images[index].image, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
#ifdef GL_FRAMEBUFFER_SRGB_EXT
    // the game's colours are already gamma-encoded: store them as they are
    glDisable(GL_FRAMEBUFFER_SRGB_EXT);
#endif
    glBlitFramebuffer(0, 0, width, height, 0, 0, g.width, g.height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    XrSwapchainImageReleaseInfo release = xrStruct<XrSwapchainImageReleaseInfo>(XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO);
    return ok(xrReleaseSwapchainImage(g.swapchain, &release), "xrReleaseSwapchainImage");
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

    int width = 0, height = 0;
    SDL_GL_GetDrawableSize(SDL_GL_GetCurrentWindow(), &width, &height);
    if(initLoader(vm, activityRef) && createInstance(vm, activityRef) && createSession()
            && createSwapchain(width, height)) {
        printf("[XR] on: the game is shown on a %.1f m screen %.1f m ahead\n", kScreenWidth, kScreenDistance);
        return true;
    }
    shutdown();
    return false;
}

bool present(int width, int height)
{
    if(g.session == XR_NULL_HANDLE)
        return false;
    pollEvents();
    if(!g.running) {
        SDL_Delay(10);  // not visible yet: no frame to pace the loop
        return true;
    }

    XrFrameWaitInfo waitInfo = xrStruct<XrFrameWaitInfo>(XR_TYPE_FRAME_WAIT_INFO);
    XrFrameState frame = xrStruct<XrFrameState>(XR_TYPE_FRAME_STATE);
    if(!ok(xrWaitFrame(g.session, &waitInfo, &frame), "xrWaitFrame"))
        return true;
    XrFrameBeginInfo beginInfo = xrStruct<XrFrameBeginInfo>(XR_TYPE_FRAME_BEGIN_INFO);
    if(!ok(xrBeginFrame(g.session, &beginInfo), "xrBeginFrame"))
        return true;

    XrCompositionLayerQuad quad = xrStruct<XrCompositionLayerQuad>(XR_TYPE_COMPOSITION_LAYER_QUAD);
    const XrCompositionLayerBaseHeader* layers[] = {(XrCompositionLayerBaseHeader*)&quad};
    uint32_t layerCount = 0;
    if(frame.shouldRender && copyFrame(width, height)) {
        quad.space = g.space;
        quad.eyeVisibility = XR_EYE_VISIBILITY_BOTH;
        quad.subImage.swapchain = g.swapchain;
        quad.subImage.imageRect.extent = {g.width, g.height};
        quad.pose.orientation.w = 1.0f;
        quad.pose.position.z = -kScreenDistance;
        quad.size.width = kScreenWidth;
        quad.size.height = kScreenWidth * (float)g.height / (float)g.width;
        layerCount = 1;
    }

    XrFrameEndInfo endInfo = xrStruct<XrFrameEndInfo>(XR_TYPE_FRAME_END_INFO);
    endInfo.displayTime = frame.predictedDisplayTime;
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

void shutdown()
{
    if(g.fbo)
        glDeleteFramebuffers(1, &g.fbo);
    if(g.swapchain != XR_NULL_HANDLE)
        xrDestroySwapchain(g.swapchain);
    if(g.space != XR_NULL_HANDLE)
        xrDestroySpace(g.space);
    if(g.session != XR_NULL_HANDLE)
        xrDestroySession(g.session);
    if(g.instance != XR_NULL_HANDLE)
        xrDestroyInstance(g.instance);
    g = State();
}

} // namespace gos_xr
