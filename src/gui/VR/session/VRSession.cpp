#include "VR/session/VRSession.hpp"

#include "Graphics/window/Window.hpp"

// clang-format off
// X11/GLX headers define macros (KeyPress, None, Status, Bool, True, False, ...) that collide
// with identifiers used throughout the rest of the codebase, so they -- and the OpenXR platform
// header that needs them for the OpenGL/GLX graphics binding -- are confined to this translation
// unit alone. glad.h (pulled in via Window.hpp above) must come first: it defines __gl_h_ so that
// GL/glx.h's own #include <GL/gl.h> becomes a no-op instead of redeclaring GL symbols.
#define XR_USE_PLATFORM_XLIB
#define XR_USE_GRAPHICS_API_OPENGL
#include <X11/Xlib.h>
#include <GL/glx.h>
#include <openxr/openxr_platform.h>
// clang-format on

#include <cstdio>
#include <cstring>

namespace Zappy
{

namespace
{

constexpr XrViewConfigurationType ViewConfig = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

XrTime secondsToXrTime(double seconds)
{
    return static_cast<XrTime>(seconds * 1e9);
}

double xrTimeToSeconds(XrTime time)
{
    return static_cast<double>(time) * 1e-9;
}

/// @brief Finds the GLXFBConfig backing the window's current GL context, for the OpenXR graphics binding.
GLXFBConfig currentFBConfig(Display *display, GLXContext context)
{
    int screen = 0;
    int fbConfigId = 0;

    glXQueryContext(display, context, GLX_SCREEN, &screen);
    glXQueryContext(display, context, GLX_FBCONFIG_ID, &fbConfigId);

    int count = 0;
    GLXFBConfig *configs = glXGetFBConfigs(display, screen, &count);
    GLXFBConfig match = nullptr;

    for (int i = 0; i < count; ++i)
    {
        int id = 0;

        glXGetFBConfigAttrib(display, configs[i], GLX_FBCONFIG_ID, &id);
        if (id == fbConfigId)
        {
            match = configs[i];
            break;
        }
    }
    if (configs != nullptr)
        XFree(configs);
    return match;
}

} // namespace

/// @brief Per-eye swapchain plus the color images/FBOs it hands out each frame.
struct VRSession::SwapchainData
{
    XrSwapchain swapchain = XR_NULL_HANDLE; ///< OpenXR swapchain for this eye.
    int width = 0;                          ///< Swapchain image width in pixels.
    int height = 0;                         ///< Swapchain image height in pixels.
    std::vector<GLuint> depthRenderbuffers; ///< One depth renderbuffer per image, owned by us.
    std::vector<GLuint> framebuffers;       ///< One FBO per image, owned by us.
};

VRSession::VRSession(Window &window)
    : _window(window), _instance(XR_NULL_HANDLE), _systemId(XR_NULL_SYSTEM_ID), _session(XR_NULL_HANDLE), _space(XR_NULL_HANDLE), _sessionState(XR_SESSION_STATE_UNKNOWN),
      _sessionRunning(false), _sessionFocused(false), _exiting(false), _enabledExtensions(), _swapchains(), _acquiredImageIndex{0, 0}
{
}

VRSession::~VRSession()
{
    destroy();
}

void VRSession::init()
{
    try
    {
        createInstance();
        createSystem();
        checkGraphicsRequirements();
        createSession();
        createReferenceSpace();
        createSwapchains();
    }
    catch (...)
    {
        destroy();
        throw;
    }
}

void VRSession::createInstance()
{
    uint32_t extensionCount = 0;

    if (XR_FAILED(xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr)))
        throw VRSessionException("Failed to reach an OpenXR runtime (none installed, or none registered for this session)");

    std::vector<XrExtensionProperties> available(extensionCount, XrExtensionProperties{XR_TYPE_EXTENSION_PROPERTIES, nullptr, {}, 0});

    if (extensionCount > 0 && XR_FAILED(xrEnumerateInstanceExtensionProperties(nullptr, extensionCount, &extensionCount, available.data())))
        throw VRSessionException("Failed to enumerate OpenXR instance extensions");

    bool hasOpenGL = false;
    bool hasHandTracking = false;

    for (const XrExtensionProperties &extension : available)
    {
        if (std::strcmp(extension.extensionName, XR_KHR_OPENGL_ENABLE_EXTENSION_NAME) == 0)
            hasOpenGL = true;
        if (std::strcmp(extension.extensionName, "XR_EXT_hand_tracking") == 0)
            hasHandTracking = true;
    }
    if (!hasOpenGL)
        throw VRSessionException("OpenXR runtime does not support XR_KHR_opengl_enable");

    std::vector<const char *> extensions{XR_KHR_OPENGL_ENABLE_EXTENSION_NAME};

    if (hasHandTracking)
        extensions.push_back("XR_EXT_hand_tracking");

    XrInstanceCreateInfo createInfo{};

    createInfo.type = XR_TYPE_INSTANCE_CREATE_INFO;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.enabledExtensionNames = extensions.data();
    std::snprintf(createInfo.applicationInfo.applicationName, XR_MAX_APPLICATION_NAME_SIZE, "Zappy");
    createInfo.applicationInfo.applicationVersion = 1;
    std::snprintf(createInfo.applicationInfo.engineName, XR_MAX_ENGINE_NAME_SIZE, "ZappyGUI");
    createInfo.applicationInfo.engineVersion = 1;
    createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;

    if (XR_FAILED(xrCreateInstance(&createInfo, &_instance)))
        throw VRSessionException("xrCreateInstance failed: no OpenXR runtime is installed or reachable");

    _enabledExtensions.assign(extensions.begin(), extensions.end());
}

void VRSession::createSystem()
{
    XrSystemGetInfo getInfo{};

    getInfo.type = XR_TYPE_SYSTEM_GET_INFO;
    getInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    if (XR_FAILED(xrGetSystem(_instance, &getInfo, &_systemId)))
        throw VRSessionException("xrGetSystem failed: no head-mounted display is available");
}

void VRSession::checkGraphicsRequirements()
{
    PFN_xrGetOpenGLGraphicsRequirementsKHR getRequirements = nullptr;

    if (XR_FAILED(xrGetInstanceProcAddr(_instance, "xrGetOpenGLGraphicsRequirementsKHR", reinterpret_cast<PFN_xrVoidFunction *>(&getRequirements))) || getRequirements == nullptr)
        throw VRSessionException("Failed to resolve xrGetOpenGLGraphicsRequirementsKHR");

    XrGraphicsRequirementsOpenGLKHR requirements{};

    requirements.type = XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR;
    if (XR_FAILED(getRequirements(_instance, _systemId, &requirements)))
        throw VRSessionException("xrGetOpenGLGraphicsRequirementsKHR failed");
    // The spec requires this call before xrCreateSession; the runtime enforces the actual
    // GL-version compatibility check on the graphics binding, so nothing else to validate here.
}

void VRSession::createSession()
{
    Display *display = static_cast<Display *>(_window.nativeDisplay());
    GLXContext context = static_cast<GLXContext>(_window.nativeGLXContext());
    GLXDrawable drawable = static_cast<GLXDrawable>(_window.nativeGLXWindow());
    GLXFBConfig fbConfig = currentFBConfig(display, context);
    int visualId = 0;

    if (fbConfig != nullptr)
    {
        XVisualInfo *visualInfo = glXGetVisualFromFBConfig(display, fbConfig);

        if (visualInfo != nullptr)
        {
            visualId = static_cast<int>(visualInfo->visualid);
            XFree(visualInfo);
        }
    }

    XrGraphicsBindingOpenGLXlibKHR binding{};

    binding.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR;
    binding.xDisplay = display;
    binding.visualid = static_cast<uint32_t>(visualId);
    binding.glxFBConfig = fbConfig;
    binding.glxDrawable = drawable;
    binding.glxContext = context;

    XrSessionCreateInfo createInfo{};

    createInfo.type = XR_TYPE_SESSION_CREATE_INFO;
    createInfo.next = &binding;
    createInfo.systemId = _systemId;
    if (XR_FAILED(xrCreateSession(_instance, &createInfo, &_session)))
        throw VRSessionException("xrCreateSession failed");
}

void VRSession::createReferenceSpace()
{
    XrReferenceSpaceCreateInfo createInfo{};

    createInfo.type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO;
    createInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    createInfo.poseInReferenceSpace.orientation = XrQuaternionf{0.0f, 0.0f, 0.0f, 1.0f};
    createInfo.poseInReferenceSpace.position = XrVector3f{0.0f, 0.0f, 0.0f};
    if (XR_FAILED(xrCreateReferenceSpace(_session, &createInfo, &_space)))
        throw VRSessionException("xrCreateReferenceSpace failed");
}

void VRSession::createSwapchains()
{
    uint32_t viewCount = 0;

    xrEnumerateViewConfigurationViews(_instance, _systemId, ViewConfig, 0, &viewCount, nullptr);

    std::vector<XrViewConfigurationView> views(viewCount, XrViewConfigurationView{XR_TYPE_VIEW_CONFIGURATION_VIEW, nullptr, 0, 0, 0, 0, 0, 0});

    if (viewCount < EyeCount || XR_FAILED(xrEnumerateViewConfigurationViews(_instance, _systemId, ViewConfig, viewCount, &viewCount, views.data())))
        throw VRSessionException("xrEnumerateViewConfigurationViews failed or returned fewer than two eyes");

    uint32_t formatCount = 0;

    xrEnumerateSwapchainFormats(_session, 0, &formatCount, nullptr);

    std::vector<int64_t> formats(formatCount, 0);

    if (XR_FAILED(xrEnumerateSwapchainFormats(_session, formatCount, &formatCount, formats.data())))
        throw VRSessionException("xrEnumerateSwapchainFormats failed");

    int64_t chosenFormat = formats.empty() ? static_cast<int64_t>(GL_RGBA8) : formats[0];

    for (int64_t format : formats)
        if (format == static_cast<int64_t>(GL_RGBA8) || format == static_cast<int64_t>(GL_SRGB8_ALPHA8))
        {
            chosenFormat = format;
            break;
        }

    for (int eye = 0; eye < EyeCount; ++eye)
    {
        auto data = std::make_unique<SwapchainData>();

        data->width = static_cast<int>(views[eye].recommendedImageRectWidth);
        data->height = static_cast<int>(views[eye].recommendedImageRectHeight);

        XrSwapchainCreateInfo createInfo{};

        createInfo.type = XR_TYPE_SWAPCHAIN_CREATE_INFO;
        createInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_SAMPLED_BIT;
        createInfo.format = chosenFormat;
        createInfo.sampleCount = 1;
        createInfo.width = static_cast<uint32_t>(data->width);
        createInfo.height = static_cast<uint32_t>(data->height);
        createInfo.faceCount = 1;
        createInfo.arraySize = 1;
        createInfo.mipCount = 1;
        if (XR_FAILED(xrCreateSwapchain(_session, &createInfo, &data->swapchain)))
            throw VRSessionException("xrCreateSwapchain failed for one eye");

        uint32_t imageCount = 0;

        xrEnumerateSwapchainImages(data->swapchain, 0, &imageCount, nullptr);

        std::vector<XrSwapchainImageOpenGLKHR> images(imageCount, XrSwapchainImageOpenGLKHR{XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR, nullptr, 0});

        if (XR_FAILED(xrEnumerateSwapchainImages(data->swapchain, imageCount, &imageCount, reinterpret_cast<XrSwapchainImageBaseHeader *>(images.data()))))
            throw VRSessionException("xrEnumerateSwapchainImages failed for one eye");

        for (const XrSwapchainImageOpenGLKHR &image : images)
        {
            GLuint depth = 0;
            GLuint fbo = 0;

            glGenRenderbuffers(1, &depth);
            glBindRenderbuffer(GL_RENDERBUFFER, depth);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, data->width, data->height);
            glGenFramebuffers(1, &fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, image.image, 0);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);
            data->depthRenderbuffers.push_back(depth);
            data->framebuffers.push_back(fbo);
        }
        _swapchains[eye] = std::move(data);
    }
}

void VRSession::destroy()
{
    for (std::unique_ptr<SwapchainData> &data : _swapchains)
    {
        if (data == nullptr)
            continue;
        for (GLuint fbo : data->framebuffers)
            glDeleteFramebuffers(1, &fbo);
        for (GLuint depth : data->depthRenderbuffers)
            glDeleteRenderbuffers(1, &depth);
        if (data->swapchain != XR_NULL_HANDLE)
            xrDestroySwapchain(data->swapchain);
        data.reset();
    }
    if (_space != XR_NULL_HANDLE)
    {
        xrDestroySpace(_space);
        _space = XR_NULL_HANDLE;
    }
    if (_session != XR_NULL_HANDLE)
    {
        if (_sessionRunning)
            xrEndSession(_session);
        xrDestroySession(_session);
        _session = XR_NULL_HANDLE;
    }
    if (_instance != XR_NULL_HANDLE)
    {
        xrDestroyInstance(_instance);
        _instance = XR_NULL_HANDLE;
    }
    _sessionRunning = false;
    _sessionFocused = false;
}

bool VRSession::pollEvents()
{
    while (true)
    {
        XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER, nullptr, {}};
        XrResult result = xrPollEvent(_instance, &event);

        if (result == XR_EVENT_UNAVAILABLE || XR_FAILED(result))
            break;
        if (event.type != XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED)
            continue;

        const auto &stateEvent = reinterpret_cast<const XrEventDataSessionStateChanged &>(event);

        _sessionState = stateEvent.state;
        switch (_sessionState)
        {
        case XR_SESSION_STATE_READY: {
            XrSessionBeginInfo beginInfo{};

            beginInfo.type = XR_TYPE_SESSION_BEGIN_INFO;
            beginInfo.primaryViewConfigurationType = ViewConfig;
            if (XR_SUCCEEDED(xrBeginSession(_session, &beginInfo)))
                _sessionRunning = true;
            break;
        }
        case XR_SESSION_STATE_STOPPING:
            if (_sessionRunning)
                xrEndSession(_session);
            _sessionRunning = false;
            _sessionFocused = false;
            break;
        case XR_SESSION_STATE_LOSS_PENDING:
        case XR_SESSION_STATE_EXITING:
            _exiting = true;
            _sessionRunning = false;
            _sessionFocused = false;
            break;
        case XR_SESSION_STATE_FOCUSED:
            _sessionFocused = true;
            break;
        default:
            _sessionFocused = false;
            break;
        }
    }
    return !_exiting;
}

bool VRSession::beginFrame(double &predictedDisplayTimeSeconds)
{
    if (!pollEvents())
        return false;

    XrFrameWaitInfo waitInfo{};
    XrFrameState frameState{};

    waitInfo.type = XR_TYPE_FRAME_WAIT_INFO;
    frameState.type = XR_TYPE_FRAME_STATE;
    if (!_sessionRunning || XR_FAILED(xrWaitFrame(_session, &waitInfo, &frameState)))
        return false;

    XrFrameBeginInfo beginInfo{};

    beginInfo.type = XR_TYPE_FRAME_BEGIN_INFO;
    if (XR_FAILED(xrBeginFrame(_session, &beginInfo)))
        return false;

    predictedDisplayTimeSeconds = xrTimeToSeconds(frameState.predictedDisplayTime);
    return frameState.shouldRender != XR_FALSE;
}

std::array<VRSession::EyeView, VRSession::EyeCount> VRSession::locateViews(double predictedDisplayTimeSeconds)
{
    XrViewLocateInfo locateInfo{};

    locateInfo.type = XR_TYPE_VIEW_LOCATE_INFO;
    locateInfo.viewConfigurationType = ViewConfig;
    locateInfo.displayTime = secondsToXrTime(predictedDisplayTimeSeconds);
    locateInfo.space = _space;

    XrViewState viewState{};

    viewState.type = XR_TYPE_VIEW_STATE;
    std::array<XrView, EyeCount> rawViews{};

    rawViews[0].type = XR_TYPE_VIEW;
    rawViews[1].type = XR_TYPE_VIEW;

    uint32_t viewCount = 0;

    xrLocateViews(_session, &locateInfo, &viewState, EyeCount, &viewCount, rawViews.data());

    std::array<EyeView, EyeCount> eyes{};

    for (int eye = 0; eye < EyeCount && eye < static_cast<int>(viewCount); ++eye)
    {
        const XrView &raw = rawViews[eye];

        eyes[eye].position = Vec3(raw.pose.position.x, raw.pose.position.y, raw.pose.position.z);
        eyes[eye].orientation = Quat(raw.pose.orientation.w, raw.pose.orientation.x, raw.pose.orientation.y, raw.pose.orientation.z);
        eyes[eye].angleLeft = raw.fov.angleLeft;
        eyes[eye].angleRight = raw.fov.angleRight;
        eyes[eye].angleUp = raw.fov.angleUp;
        eyes[eye].angleDown = raw.fov.angleDown;
    }
    return eyes;
}

VRSession::EyeTarget VRSession::acquireEye(int eyeIndex)
{
    SwapchainData &data = *_swapchains[static_cast<std::size_t>(eyeIndex)];
    XrSwapchainImageAcquireInfo acquireInfo{};
    uint32_t index = 0;

    acquireInfo.type = XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO;
    xrAcquireSwapchainImage(data.swapchain, &acquireInfo, &index);

    XrSwapchainImageWaitInfo waitInfo{};

    waitInfo.type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO;
    waitInfo.timeout = XR_INFINITE_DURATION;
    xrWaitSwapchainImage(data.swapchain, &waitInfo);
    _acquiredImageIndex[static_cast<std::size_t>(eyeIndex)] = static_cast<int>(index);
    return EyeTarget{data.framebuffers[index], data.width, data.height};
}

void VRSession::releaseEye(int eyeIndex)
{
    SwapchainData &data = *_swapchains[static_cast<std::size_t>(eyeIndex)];
    XrSwapchainImageReleaseInfo releaseInfo{};

    releaseInfo.type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO;
    xrReleaseSwapchainImage(data.swapchain, &releaseInfo);
}

void VRSession::endFrame(double predictedDisplayTimeSeconds, const std::array<EyeView, EyeCount> &views)
{
    bool shouldRender = _sessionRunning;
    std::array<XrCompositionLayerProjectionView, EyeCount> projViews{};

    for (int eye = 0; eye < EyeCount; ++eye)
    {
        projViews[eye].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
        projViews[eye].pose.position = XrVector3f{views[eye].position.x, views[eye].position.y, views[eye].position.z};
        projViews[eye].pose.orientation = XrQuaternionf{views[eye].orientation.x, views[eye].orientation.y, views[eye].orientation.z, views[eye].orientation.w};
        projViews[eye].fov = XrFovf{views[eye].angleLeft, views[eye].angleRight, views[eye].angleUp, views[eye].angleDown};
        projViews[eye].subImage.swapchain = _swapchains[static_cast<std::size_t>(eye)]->swapchain;
        projViews[eye].subImage.imageRect =
            XrRect2Di{XrOffset2Di{0, 0}, XrExtent2Di{_swapchains[static_cast<std::size_t>(eye)]->width, _swapchains[static_cast<std::size_t>(eye)]->height}};
        projViews[eye].subImage.imageArrayIndex = 0;
    }

    XrCompositionLayerProjection layer{};

    layer.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION;
    layer.space = _space;
    layer.viewCount = EyeCount;
    layer.views = projViews.data();

    const XrCompositionLayerBaseHeader *layers[1] = {reinterpret_cast<const XrCompositionLayerBaseHeader *>(&layer)};
    XrFrameEndInfo endInfo{};

    endInfo.type = XR_TYPE_FRAME_END_INFO;
    endInfo.displayTime = secondsToXrTime(predictedDisplayTimeSeconds);
    endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    endInfo.layerCount = shouldRender ? 1 : 0;
    endInfo.layers = shouldRender ? layers : nullptr;
    xrEndFrame(_session, &endInfo);
}

bool VRSession::isSessionRunning() const
{
    return _sessionFocused;
}

bool VRSession::wantsExit() const
{
    return _exiting;
}

bool VRSession::extensionEnabled(const std::string &name) const
{
    for (const std::string &extension : _enabledExtensions)
        if (extension == name)
            return true;
    return false;
}

XrInstance VRSession::instanceHandle() const
{
    return _instance;
}

XrSession VRSession::sessionHandle() const
{
    return _session;
}

XrSpace VRSession::spaceHandle() const
{
    return _space;
}

} // namespace Zappy
