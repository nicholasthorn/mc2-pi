// OpenXR presentation for the Meta Quest build: see gos_xr.h.
//
// The game draws as before into the default framebuffer (SDL's window
// surface): the whole frame, interface included. In VR the camera also
// draws each eye into its own swapchain image (bindEye) during that frame.
// present() copies the window's frame into the quad's swapchain and submits
// the eyes as a projection layer with the quad on top, in LOCAL space
// (origin where the head was when the session started).
//
// Controllers (pollInput, every frame): the pointing hand's ray becomes the
// mouse where it meets the quad, the sticks and buttons become keys (see
// gos_xr.h for the mapping), through GameOS's virtual mouse and keyboard.

#include "gos_xr.h"

#include "gameos.hpp"
#include "utils/graphics.h"
#include "utils/gl_utils.h"
#include "utils/tga_writer.h"
#include "utils/vr_controls.h"

#include <EGL/egl.h>
#include <SDL2/SDL.h>
#include <jni.h>
#include "gos_input.h"      // after SDL, which it uses

#define XR_USE_PLATFORM_ANDROID
#define XR_USE_GRAPHICS_API_OPENGL_ES
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include <algorithm>
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

// the pointer stays this many pixels inside the window's edges, where the
// game would scroll the map (the sticks do that)
const float kPointerMargin = 8.0f;

// the controllers' buttons that are not per hand
enum Button { BUTTON_A, BUTTON_B, BUTTON_X, BUTTON_Y, BUTTON_MENU, BUTTON_COUNT };

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
    // the quad: placed at hudYaw from hudOrigin (MC2_XR_HUD, button A)
    XrSpace viewSpace = XR_NULL_HANDLE;     // the head
    float hudOrigin[3] = {0.0f, 0.0f, 0.0f};
    float hudYaw = 0.0f;
    float hudDistance = kHudDistance;
    float hudDown = kHudDown;
    float hudWidth = kHudWidth;
    vr_controls::Pose hud = {};
    bool hudAdjusted = false;
    // controllers: 0 left hand, 1 right
    XrActionSet actionSet = XR_NULL_HANDLE;
    XrPath hands[2] = {XR_NULL_PATH, XR_NULL_PATH};
    XrAction aimAction = XR_NULL_HANDLE;
    XrAction selectAction = XR_NULL_HANDLE;
    XrAction contextAction = XR_NULL_HANDLE;
    XrAction stickAction = XR_NULL_HANDLE;
    XrAction buttonActions[BUTTON_COUNT] = {};
    XrSpace aimSpaces[2] = {XR_NULL_HANDLE, XR_NULL_HANDLE};
    int pointingHand = 1;
    bool selectDown[2] = {false, false};
    bool contextDown[2] = {false, false};
    bool buttonDown[BUTTON_COUNT] = {};
    uint32_t stickActions = 0;
    bool pointerHeld = false;       // a button went down with the ray on the quad
    uint32_t heldButtons = 0;       // the pointing hand's, as mouse button bits
    uint32_t ignoredButtons = 0;    // pressed off the quad, not yet let go
    float pointerX = 0.0f, pointerY = 0.0f;
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
    if(!ok(xrCreateReferenceSpace(g.session, &spaceInfo, &g.space), "xrCreateReferenceSpace"))
        return false;
    spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
    return ok(xrCreateReferenceSpace(g.session, &spaceInfo, &g.viewSpace), "xrCreateReferenceSpace(VIEW)");
}

XrPath toPath(const char* name)
{
    XrPath path = XR_NULL_PATH;
    ok(xrStringToPath(g.instance, name, &path), name);
    return path;
}

XrAction createAction(XrActionType type, const char* name, const char* localizedName, bool perHand)
{
    XrActionCreateInfo info = xrStruct<XrActionCreateInfo>(XR_TYPE_ACTION_CREATE_INFO);
    info.actionType = type;
    strncpy(info.actionName, name, XR_MAX_ACTION_NAME_SIZE - 1);
    strncpy(info.localizedActionName, localizedName, XR_MAX_LOCALIZED_ACTION_NAME_SIZE - 1);
    if(perHand) {
        info.countSubactionPaths = 2;
        info.subactionPaths = g.hands;
    }
    XrAction action = XR_NULL_HANDLE;
    ok(xrCreateAction(g.actionSet, &info, &action), name);
    return action;
}

// The Touch controllers' actions, bound for the Quest. False leaves the game
// without controllers (the session still runs).
bool createActions()
{
    g.hands[0] = toPath("/user/hand/left");
    g.hands[1] = toPath("/user/hand/right");

    XrActionSetCreateInfo setInfo = xrStruct<XrActionSetCreateInfo>(XR_TYPE_ACTION_SET_CREATE_INFO);
    strncpy(setInfo.actionSetName, "gameplay", XR_MAX_ACTION_SET_NAME_SIZE - 1);
    strncpy(setInfo.localizedActionSetName, "Gameplay", XR_MAX_LOCALIZED_ACTION_SET_NAME_SIZE - 1);
    if(!ok(xrCreateActionSet(g.instance, &setInfo, &g.actionSet), "xrCreateActionSet"))
        return false;

    g.aimAction = createAction(XR_ACTION_TYPE_POSE_INPUT, "point", "Point", true);
    g.selectAction = createAction(XR_ACTION_TYPE_FLOAT_INPUT, "select", "Select (left mouse)", true);
    g.contextAction = createAction(XR_ACTION_TYPE_FLOAT_INPUT, "context", "Context (right mouse)", true);
    g.stickAction = createAction(XR_ACTION_TYPE_VECTOR2F_INPUT, "camera", "Camera", true);
    g.buttonActions[BUTTON_A] = createAction(XR_ACTION_TYPE_BOOLEAN_INPUT, "place_hud", "Place the interface", false);
    g.buttonActions[BUTTON_B] = createAction(XR_ACTION_TYPE_BOOLEAN_INPUT, "ctrl", "Ctrl", false);
    g.buttonActions[BUTTON_X] = createAction(XR_ACTION_TYPE_BOOLEAN_INPUT, "shift", "Shift", false);
    g.buttonActions[BUTTON_Y] = createAction(XR_ACTION_TYPE_BOOLEAN_INPUT, "alt", "Alt", false);
    g.buttonActions[BUTTON_MENU] = createAction(XR_ACTION_TYPE_BOOLEAN_INPUT, "escape", "Escape", false);
    if(!g.aimAction || !g.selectAction || !g.contextAction || !g.stickAction)
        return false;
    for(XrAction action : g.buttonActions)
        if(!action)
            return false;

    const struct { XrAction action; const char* path; } bindings[] = {
        {g.aimAction, "/user/hand/left/input/aim/pose"},
        {g.aimAction, "/user/hand/right/input/aim/pose"},
        {g.selectAction, "/user/hand/left/input/trigger/value"},
        {g.selectAction, "/user/hand/right/input/trigger/value"},
        {g.contextAction, "/user/hand/left/input/squeeze/value"},
        {g.contextAction, "/user/hand/right/input/squeeze/value"},
        {g.stickAction, "/user/hand/left/input/thumbstick"},
        {g.stickAction, "/user/hand/right/input/thumbstick"},
        {g.buttonActions[BUTTON_A], "/user/hand/right/input/a/click"},
        {g.buttonActions[BUTTON_B], "/user/hand/right/input/b/click"},
        {g.buttonActions[BUTTON_X], "/user/hand/left/input/x/click"},
        {g.buttonActions[BUTTON_Y], "/user/hand/left/input/y/click"},
        {g.buttonActions[BUTTON_MENU], "/user/hand/left/input/menu/click"},
    };
    std::vector<XrActionSuggestedBinding> suggested;
    for(const auto& b : bindings)
        suggested.push_back({b.action, toPath(b.path)});
    XrInteractionProfileSuggestedBinding profile = xrStruct<XrInteractionProfileSuggestedBinding>(XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING);
    profile.interactionProfile = toPath("/interaction_profiles/oculus/touch_controller");
    profile.countSuggestedBindings = (uint32_t)suggested.size();
    profile.suggestedBindings = suggested.data();
    if(!ok(xrSuggestInteractionProfileBindings(g.instance, &profile), "xrSuggestInteractionProfileBindings"))
        return false;

    XrSessionActionSetsAttachInfo attach = xrStruct<XrSessionActionSetsAttachInfo>(XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO);
    attach.countActionSets = 1;
    attach.actionSets = &g.actionSet;
    if(!ok(xrAttachSessionActionSets(g.session, &attach), "xrAttachSessionActionSets"))
        return false;

    for(int hand = 0; hand < 2; ++hand) {
        XrActionSpaceCreateInfo space = xrStruct<XrActionSpaceCreateInfo>(XR_TYPE_ACTION_SPACE_CREATE_INFO);
        space.action = g.aimAction;
        space.subactionPath = g.hands[hand];
        space.poseInActionSpace.orientation.w = 1.0f;
        if(!ok(xrCreateActionSpace(g.session, &space, &g.aimSpaces[hand]), "xrCreateActionSpace"))
            return false;
    }
    return true;
}

float floatInput(XrAction action, XrPath hand)
{
    XrActionStateGetInfo info = xrStruct<XrActionStateGetInfo>(XR_TYPE_ACTION_STATE_GET_INFO);
    info.action = action;
    info.subactionPath = hand;
    XrActionStateFloat state = xrStruct<XrActionStateFloat>(XR_TYPE_ACTION_STATE_FLOAT);
    if(XR_FAILED(xrGetActionStateFloat(g.session, &info, &state)) || !state.isActive)
        return 0.0f;
    return state.currentState;
}

bool boolInput(XrAction action)
{
    XrActionStateGetInfo info = xrStruct<XrActionStateGetInfo>(XR_TYPE_ACTION_STATE_GET_INFO);
    info.action = action;
    XrActionStateBoolean state = xrStruct<XrActionStateBoolean>(XR_TYPE_ACTION_STATE_BOOLEAN);
    if(XR_FAILED(xrGetActionStateBoolean(g.session, &info, &state)) || !state.isActive)
        return false;
    return state.currentState == XR_TRUE;
}

void stickInput(XrPath hand, float stick[2])
{
    stick[0] = stick[1] = 0.0f;
    XrActionStateGetInfo info = xrStruct<XrActionStateGetInfo>(XR_TYPE_ACTION_STATE_GET_INFO);
    info.action = g.stickAction;
    info.subactionPath = hand;
    XrActionStateVector2f state = xrStruct<XrActionStateVector2f>(XR_TYPE_ACTION_STATE_VECTOR2F);
    if(XR_FAILED(xrGetActionStateVector2f(g.session, &info, &state)) || !state.isActive)
        return;
    stick[0] = state.currentState.x;
    stick[1] = state.currentState.y;
}

// A space's pose in the LOCAL space at the frame's display time.
bool locate(XrSpace space, vr_controls::Pose& pose)
{
    if(space == XR_NULL_HANDLE || g.frame.predictedDisplayTime == 0)
        return false;
    XrSpaceLocation location = xrStruct<XrSpaceLocation>(XR_TYPE_SPACE_LOCATION);
    if(XR_FAILED(xrLocateSpace(space, g.space, g.frame.predictedDisplayTime, &location)))
        return false;
    const XrSpaceLocationFlags valid = XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
    if((location.locationFlags & valid) != valid)
        return false;
    pose.position[0] = location.pose.position.x;
    pose.position[1] = location.pose.position.y;
    pose.position[2] = location.pose.position.z;
    pose.orientation[0] = location.pose.orientation.x;
    pose.orientation[1] = location.pose.orientation.y;
    pose.orientation[2] = location.pose.orientation.z;
    pose.orientation[3] = location.pose.orientation.w;
    return true;
}

void placeHud()
{
    g.hud = vr_controls::hudPose(g.hudOrigin, g.hudYaw, g.hudDistance, g.hudDown);
}

// The quad in front of where the head is and faces, at the set distance and
// angle below the horizon.
void recentreHud()
{
    vr_controls::Pose head;
    if(!locate(g.viewSpace, head))
        return;
    memcpy(g.hudOrigin, head.position, sizeof(g.hudOrigin));
    g.hudYaw = vr_controls::yawOf(head.orientation);
    placeHud();
}

// MC2_XR_HUD=distance,degrees,width: the quad's distance (metres), angle
// below the horizon (degrees) and width (metres); any can be left out
void readHudSettings()
{
    g.hudDistance = g.useEyes ? kHudDistance : kScreenDistance;
    g.hudDown = g.useEyes ? kHudDown : 0.0f;
    g.hudWidth = g.useEyes ? kHudWidth : kScreenWidth;
    const char* env = getenv("MC2_XR_HUD");
    float distance = 0.0f, degrees = 0.0f, width = 0.0f;
    const int count = env ? sscanf(env, "%f,%f,%f", &distance, &degrees, &width) : 0;
    if(count >= 1 && distance > 0.0f)
        g.hudDistance = distance;
    if(count >= 2)
        g.hudDown = degrees * 3.14159265f / 180.0f;
    if(count >= 3 && width > 0.0f)
        g.hudWidth = width;
    placeHud();
}

// Holding A: the sticks move the quad instead of the camera. Left stick
// up/down: further/nearer; right stick up/down: higher/lower; right stick
// left/right: narrower/wider.
void adjustHud(const float left[2], const float right[2])
{
    const float dt = g.frame.predictedDisplayPeriod > 0 ? (float)g.frame.predictedDisplayPeriod * 1e-9f : 1.0f / 72.0f;
    const float deadZone = 0.2f;
    auto rate = [deadZone](float v) { return std::fabs(v) < deadZone ? 0.0f : v; };
    g.hudDistance = std::min(4.0f, std::max(0.5f, g.hudDistance + 0.5f * rate(left[1]) * dt));
    g.hudDown = std::min(1.2f, std::max(-0.3f, g.hudDown - 0.5f * rate(right[1]) * dt));
    g.hudWidth = std::min(4.0f, std::max(0.5f, g.hudWidth + 0.5f * rate(right[0]) * dt));
    if(rate(left[1]) != 0.0f || rate(right[0]) != 0.0f || rate(right[1]) != 0.0f) {
        g.hudAdjusted = true;
        placeHud();
    }
}

void setKeys(uint32_t camera, const bool buttons[BUTTON_COUNT])
{
    using namespace vr_controls;
    input::setVirtualKey(SDL_SCANCODE_UP, (camera & PAN_UP) != 0);
    input::setVirtualKey(SDL_SCANCODE_DOWN, (camera & PAN_DOWN) != 0);
    input::setVirtualKey(SDL_SCANCODE_LEFT, (camera & (PAN_LEFT | ROTATE_LEFT)) != 0);
    input::setVirtualKey(SDL_SCANCODE_RIGHT, (camera & (PAN_RIGHT | ROTATE_RIGHT)) != 0);
    // the game turns the camera on shift + left / right
    input::setVirtualKey(SDL_SCANCODE_LSHIFT, (camera & (ROTATE_LEFT | ROTATE_RIGHT)) != 0 || buttons[BUTTON_X]);
    input::setVirtualKey(SDL_SCANCODE_KP_PLUS, (camera & ZOOM_IN) != 0);
    input::setVirtualKey(SDL_SCANCODE_KP_MINUS, (camera & ZOOM_OUT) != 0);
    input::setVirtualKey(SDL_SCANCODE_LCTRL, buttons[BUTTON_B]);
    input::setVirtualKey(SDL_SCANCODE_LALT, buttons[BUTTON_Y]);
    input::setVirtualKey(SDL_SCANCODE_ESCAPE, buttons[BUTTON_MENU]);
}

// Everything up: the session lost focus (system menu, Guardian) or ended.
void releaseInput()
{
    const bool none[BUTTON_COUNT] = {};
    setKeys(0, none);
    input::clearVirtualPointer();
    g.stickActions = 0;
    g.pointerHeld = false;
    g.heldButtons = g.ignoredButtons = 0;
    for(int hand = 0; hand < 2; ++hand)
        g.selectDown[hand] = g.contextDown[hand] = false;
    memset(g.buttonDown, 0, sizeof(g.buttonDown));
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
    XrPosef pose;
    pose.position = {g.hud.position[0], g.hud.position[1], g.hud.position[2]};
    pose.orientation = {g.hud.orientation[0], g.hud.orientation[1], g.hud.orientation[2], g.hud.orientation[3]};
    return pose;
}

// the quad's height: the game's screen shape, stretched over the whole window
float quadHeight()
{
    return g.hudWidth * (float)Environment.screenHeight / (float)Environment.screenWidth;
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
        readHudSettings();
        if(createActions()) {
            printf("[XR] controllers: trigger selects, grip is the right mouse button, "
                   "left stick pans, right stick turns and zooms, A places the interface\n");
        } else {
            printf("[XR] no controllers\n");
            if(g.actionSet != XR_NULL_HANDLE)
                xrDestroyActionSet(g.actionSet);
            g.actionSet = XR_NULL_HANDLE;
        }
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
        quad.size.width = g.hudWidth;
        quad.size.height = quadHeight();
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

void pollInput()
{
    if(g.actionSet == XR_NULL_HANDLE)
        return;
    if(!g.running) {
        releaseInput();
        return;
    }
    XrActiveActionSet active = {g.actionSet, XR_NULL_PATH};
    XrActionsSyncInfo sync = xrStruct<XrActionsSyncInfo>(XR_TYPE_ACTIONS_SYNC_INFO);
    sync.countActiveActionSets = 1;
    sync.activeActionSets = &active;
    // XR_SESSION_NOT_FOCUSED: the system menu or the Guardian has the
    // controllers; let go of everything until the game has them back
    if(xrSyncActions(g.session, &sync) != XR_SUCCESS) {
        releaseInput();
        return;
    }

    // the hand that last pulled its trigger or grip points
    for(int hand = 0; hand < 2; ++hand) {
        const float select = floatInput(g.selectAction, g.hands[hand]);
        const float context = floatInput(g.contextAction, g.hands[hand]);
        const bool selectDown = select > (g.selectDown[hand] ? 0.35f : 0.6f);
        const bool contextDown = context > (g.contextDown[hand] ? 0.35f : 0.6f);
        if((selectDown && !g.selectDown[hand]) || (contextDown && !g.contextDown[hand]))
            g.pointingHand = hand;
        g.selectDown[hand] = selectDown;
        g.contextDown[hand] = contextDown;
    }

    bool buttons[BUTTON_COUNT];
    for(int b = 0; b < BUTTON_COUNT; ++b)
        buttons[b] = boolInput(g.buttonActions[b]);
    float left[2], right[2];
    stickInput(g.hands[0], left);
    stickInput(g.hands[1], right);

    // A: the quad goes in front of the head; held, the sticks move it
    if(buttons[BUTTON_A] && !g.buttonDown[BUTTON_A]) {
        recentreHud();
        g.hudAdjusted = false;
    }
    if(buttons[BUTTON_A]) {
        adjustHud(left, right);
        g.stickActions = 0;
    } else {
        if(g.buttonDown[BUTTON_A] && g.hudAdjusted)
            printf("[XR] interface placed: MC2_XR_HUD=%.2f,%.0f,%.2f\n", g.hudDistance,
                    g.hudDown * 180.0f / 3.14159265f, g.hudWidth);
        g.stickActions = vr_controls::stickActions(left, right, g.stickActions);
    }
    memcpy(g.buttonDown, buttons, sizeof(g.buttonDown));
    setKeys(g.stickActions, buttons);

    // the pointer: where the pointing hand's ray meets the quad
    const int hand = g.pointingHand;
    const uint32_t held = (g.selectDown[hand] ? 1u : 0u) | (g.contextDown[hand] ? 4u : 0u);
    vr_controls::Pose aim;
    float u = 0.0f, v = 0.0f;
    const bool onQuad = locate(g.aimSpaces[hand], aim)
            && vr_controls::rayHitsQuad(aim, g.hud, g.hudWidth, quadHeight(), u, v);
    // a button pressed off the quad stays ignored until it is let go: else
    // sweeping the ray onto the panel with the trigger held would click there
    if(!onQuad && !g.pointerHeld)
        g.ignoredButtons |= held & ~g.heldButtons;
    g.ignoredButtons &= held;
    g.heldButtons = held;
    const uint32_t mouseButtons = held & ~g.ignoredButtons;
    if(onQuad) {
        const float w = (float)Environment.drawableWidth, h = (float)Environment.drawableHeight;
        g.pointerX = std::min(w - kPointerMargin, std::max(kPointerMargin, u * w));
        g.pointerY = std::min(h - kPointerMargin, std::max(kPointerMargin, v * h));
        input::setVirtualPointer(g.pointerX, g.pointerY, mouseButtons);
        g.pointerHeld = mouseButtons != 0;
    } else if(g.pointerHeld && mouseButtons) {
        // dragged off the quad: the cursor stays where the ray left it
        // until the button is let go
        input::setVirtualPointer(g.pointerX, g.pointerY, mouseButtons);
    } else {
        // the world (picking there is still to come) or nowhere: no clicks
        input::clearVirtualPointer();
        g.pointerHeld = false;
    }
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
    for(XrSpace space : g.aimSpaces)
        if(space != XR_NULL_HANDLE)
            xrDestroySpace(space);
    if(g.actionSet != XR_NULL_HANDLE)
        xrDestroyActionSet(g.actionSet);
    if(g.viewSpace != XR_NULL_HANDLE)
        xrDestroySpace(g.viewSpace);
    if(g.space != XR_NULL_HANDLE)
        xrDestroySpace(g.space);
    if(g.session != XR_NULL_HANDLE)
        xrDestroySession(g.session);
    if(g.instance != XR_NULL_HANDLE)
        xrDestroyInstance(g.instance);
    g = State();
}

} // namespace gos_xr
