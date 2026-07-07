#pragma once

// clang-format off
#include <glad/glad.h>
#include <openxr/openxr.h>
// clang-format on

#include <array>
#include <exception>
#include <memory>
#include <string>
#include <vector>

#include "types/Mat.hpp"
#include "types/Vec.hpp"

namespace Zappy
{

class Window;

/**
 * @class VRSession
 * @brief Owns the OpenXR instance/session/swapchains and drives the per-frame VR loop.
 *
 * The only class that talks to the OpenXR runtime. Session creation shares the OpenGL context
 * already owned by Window (XR_KHR_opengl_enable, X11/GLX graphics binding). The X11/GLX-specific
 * details live only in VRSession.cpp: X11 headers define macros (KeyPress, None, Status, ...)
 * that collide with identifiers used throughout the rest of the codebase, so nothing outside this
 * one translation unit may include them. This header only ever touches openxr.h, which is
 * platform-agnostic (every handle is an opaque pointer).
 */
class VRSession
{
  public:
    /**
     * @class VRSessionException
     * @brief Raised when the OpenXR runtime/headset is unavailable or session setup fails.
     *
     * Always caught by the caller (RenderSystem::initVR): VR is a best-effort layer that must
     * fall back to desktop rendering, never a fatal error.
     */
    class VRSessionException : public std::exception
    {
      public:
        explicit VRSessionException(const std::string &message) : _message(message)
        {
        }

        const char *what() const noexcept override
        {
            return _message.c_str();
        }

      private:
        std::string _message; ///< Error description.
    };

    static constexpr int EyeCount = 2; ///< Left (index 0) and right (index 1) eye.

    /**
     * @struct EyeView
     * @brief One eye's predicted pose and asymmetric field of view for the current frame.
     */
    struct EyeView
    {
        Vec3 position;    ///< Eye position relative to the session's reference space.
        Quat orientation; ///< Eye orientation relative to the reference space.
        float angleLeft;  ///< Left half-angle of the field of view, radians (negative).
        float angleRight; ///< Right half-angle of the field of view, radians (positive).
        float angleUp;    ///< Upper half-angle of the field of view, radians (positive).
        float angleDown;  ///< Lower half-angle of the field of view, radians (negative).
    };

    /**
     * @struct EyeTarget
     * @brief Render target handed back by acquireEye(), ready to be drawn into.
     */
    struct EyeTarget
    {
        GLuint framebuffer; ///< FBO wrapping the swapchain image for this eye.
        int width;          ///< Swapchain image width in pixels.
        int height;         ///< Swapchain image height in pixels.
    };

    /**
     * @brief Binds the session to an existing window's OpenGL context (no OpenXR calls yet).
     * @param window Window whose GL context and native X11/GLX handles are shared with OpenXR.
     */
    explicit VRSession(Window &window);

    /** @brief Tears down swapchains, session and instance, in that order, if they were created. */
    ~VRSession();

    VRSession(const VRSession &) = delete;
    VRSession &operator=(const VRSession &) = delete;
    VRSession(VRSession &&) = delete;
    VRSession &operator=(VRSession &&) = delete;

    /**
     * @brief Creates the OpenXR instance, session, reference space and per-eye swapchains.
     * @throws VRSessionException If no runtime/headset is available or any setup step fails.
     */
    void init();

    /**
     * @brief Pumps the OpenXR event queue and waits for the runtime's next frame slot.
     *
     * Must be called exactly once per frame, before locateViews()/acquireEye(). Always calls
     * xrWaitFrame/xrBeginFrame to keep the session's frame-timing contract alive even when the
     * session isn't visible yet, but reports that the caller should skip rendering in that case.
     * @param predictedDisplayTimeSeconds Receives the time to predict poses for this frame.
     * @return False if the frame should not be rendered (session not visible, or exiting).
     */
    bool beginFrame(double &predictedDisplayTimeSeconds);

    /**
     * @brief Locates both eyes for the given predicted display time.
     * @param predictedDisplayTimeSeconds Time returned by beginFrame().
     * @return The two eye poses/fovs, index 0 = left, index 1 = right.
     */
    std::array<EyeView, EyeCount> locateViews(double predictedDisplayTimeSeconds);

    /**
     * @brief Acquires the next swapchain image for an eye and binds its FBO as the draw target.
     * @param eyeIndex 0 for left, 1 for right.
     * @return The FBO and its pixel dimensions, ready to render into.
     */
    EyeTarget acquireEye(int eyeIndex);

    /**
     * @brief Releases the swapchain image acquired for an eye; call once done rendering it.
     * @param eyeIndex 0 for left, 1 for right.
     */
    void releaseEye(int eyeIndex);

    /**
     * @brief Submits both eyes' rendered images to the compositor, ending the frame.
     * @param predictedDisplayTimeSeconds Time passed to beginFrame() for this frame.
     * @param views Eye poses/fovs returned by locateViews() for this frame.
     */
    void endFrame(double predictedDisplayTimeSeconds, const std::array<EyeView, EyeCount> &views);

    /**
     * @brief Whether the session has reached a state where frames are actually presented.
     * @return True once the session is visible/focused (not just synchronized).
     */
    bool isSessionRunning() const;

    /**
     * @brief Whether the runtime asked the application to stop (EXITING/LOSS_PENDING).
     * @return True if the caller should tear down VR and fall back to desktop rendering.
     */
    bool wantsExit() const;

    /**
     * @brief Whether an optional OpenXR instance extension was enabled at init() time.
     * @param name Extension name (e.g. "XR_EXT_hand_tracking").
     * @return True if the runtime advertised and we enabled it.
     */
    bool extensionEnabled(const std::string &name) const;

    /** @brief The OpenXR instance, for VRInput/VRHandTracking to build actions on. @return The instance handle. */
    XrInstance instanceHandle() const;

    /** @brief The OpenXR session, for VRInput/VRHandTracking to attach action sets to. @return The session handle. */
    XrSession sessionHandle() const;

    /** @brief The reference space poses and rays are expressed in. @return The space handle. */
    XrSpace spaceHandle() const;

  private:
    /** @brief Creates the XrInstance, requesting XR_KHR_opengl_enable and any optional extensions available. */
    void createInstance();

    /** @brief Resolves the HMD system id via xrGetSystem. @throws VRSessionException If no HMD form factor is present. */
    void createSystem();

    /** @brief Checks the runtime's OpenGL version requirements via xrGetOpenGLGraphicsRequirementsKHR. */
    void checkGraphicsRequirements();

    /** @brief Creates the session with the X11/GLX OpenGL graphics binding (built in VRSession.cpp only). */
    void createSession();

    /** @brief Creates the LOCAL reference space frame poses/rays are expressed in. */
    void createReferenceSpace();

    /** @brief Creates one swapchain per eye, sized from the runtime's recommended view configuration. */
    void createSwapchains();

    /** @brief Destroys swapchains, space, session and instance, in that order, ignoring already-null handles. */
    void destroy();

    /**
     * @brief Drains pending OpenXR events, updating the session state machine.
     * @return False if the runtime asked to exit (EXITING/LOSS_PENDING) and VR should shut down.
     */
    bool pollEvents();

    struct SwapchainData; ///< Per-eye swapchain + its acquired images' FBOs (defined in the .cpp).

    Window &_window;                                  ///< Borrowed window sharing its GL context with OpenXR.
    XrInstance _instance;                              ///< OpenXR instance (XR_NULL_HANDLE until init()).
    XrSystemId _systemId;                               ///< Resolved HMD system id.
    XrSession _session;                                 ///< OpenXR session (XR_NULL_HANDLE until createSession()).
    XrSpace _space;                                      ///< Reference space (LOCAL) poses/rays are expressed in.
    XrSessionState _sessionState;                       ///< Current session state (from XrEventDataSessionStateChanged).
    bool _sessionRunning;                                ///< True once xrBeginSession succeeded (until xrEndSession).
    bool _sessionFocused;                                ///< True once the session reached FOCUSED (visible + has input).
    bool _exiting;                                       ///< True once the runtime asked to stop (EXITING/LOSS_PENDING).
    std::vector<std::string> _enabledExtensions;         ///< Instance extensions actually enabled at init() time.
    std::array<std::unique_ptr<SwapchainData>, EyeCount> _swapchains; ///< Per-eye swapchain + FBO cache.
    std::array<int, EyeCount> _acquiredImageIndex;      ///< Swapchain image index acquired this frame, per eye.
};

} // namespace Zappy
