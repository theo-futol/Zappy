#pragma once

#include <openxr/openxr.h>

#include <array>
#include <exception>
#include <string>

namespace Zappy
{

class VRSession;

/**
 * @class VRHandTracking
 * @brief Optional XR_EXT_hand_tracking wrapper synthesizing a pinch-based "select" signal.
 *
 * Feature-detected: init() throws if the runtime didn't advertise (and VRSession didn't enable)
 * XR_EXT_hand_tracking, so callers simply skip building/using this class when it's not available.
 * The controller trigger (VRInput::selectPressed) remains the guaranteed input path on hardware
 * without hand tracking. Downstream picking/HUD code only ever sees a boolean "is this hand
 * pinching", so it never needs to know whether the signal came from a trigger or a pinch.
 */
class VRHandTracking
{
  public:
    /**
     * @class VRHandTrackingException
     * @brief Raised when the extension isn't enabled or a hand tracker can't be created.
     */
    class VRHandTrackingException : public std::exception
    {
      public:
        explicit VRHandTrackingException(const std::string &message) : _message(message)
        {
        }

        const char *what() const noexcept override
        {
            return _message.c_str();
        }

      private:
        std::string _message; ///< Error description.
    };

    static constexpr int HandCount = 2;                     ///< Left (0) and right (1) hand.
    static constexpr int LeftHand = 0;                       ///< Index of the left hand.
    static constexpr int RightHand = 1;                      ///< Index of the right hand.
    static constexpr float PinchThresholdMeters = 0.02f;     ///< Thumb/index fingertip distance below which a pinch is active.

    /**
     * @brief Binds to a session (creates no OpenXR objects yet).
     * @param session VR session whose instance/session the hand trackers are created against.
     */
    explicit VRHandTracking(VRSession &session);

    /** @brief Destroys the per-hand hand trackers. */
    ~VRHandTracking();

    VRHandTracking(const VRHandTracking &) = delete;
    VRHandTracking &operator=(const VRHandTracking &) = delete;
    VRHandTracking(VRHandTracking &&) = delete;
    VRHandTracking &operator=(VRHandTracking &&) = delete;

    /**
     * @brief Resolves the extension functions and creates the per-hand hand trackers.
     * @throws VRHandTrackingException If the extension wasn't enabled or tracker creation fails.
     */
    void init();

    /**
     * @brief Locates both hands' joints for the current frame and updates the pinch state.
     * @param predictedDisplayTimeSeconds Time returned by VRSession::beginFrame() this frame.
     */
    void locate(double predictedDisplayTimeSeconds);

    /** @brief Whether the hand's thumb and index fingertips are close enough to count as a pinch. @param hand LeftHand or RightHand. @return True if pinching. */
    bool isPinching(int hand) const;

    /** @brief Whether the hand's pinch started this frame (rising edge), for the same "select" semantics as VRInput::selectJustPressed. @param hand LeftHand or RightHand. @return True on the frame the pinch starts only. */
    bool pinchJustStarted(int hand) const;

  private:
    VRSession &_session;                            ///< Session the hand trackers are attached to.
    PFN_xrCreateHandTrackerEXT _createHandTracker;   ///< Resolved via xrGetInstanceProcAddr.
    PFN_xrDestroyHandTrackerEXT _destroyHandTracker; ///< Resolved via xrGetInstanceProcAddr.
    PFN_xrLocateHandJointsEXT _locateHandJoints;     ///< Resolved via xrGetInstanceProcAddr.
    std::array<XrHandTrackerEXT, HandCount> _trackers; ///< Per-hand tracker handle.
    std::array<bool, HandCount> _pinching;             ///< Latest pinch state, per hand.
    std::array<bool, HandCount> _pinchingPrev;         ///< Pinch state from the previous locate(), for edge detection.
};

} // namespace Zappy
