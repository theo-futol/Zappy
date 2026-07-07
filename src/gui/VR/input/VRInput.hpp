#pragma once

#include <openxr/openxr.h>

#include <array>
#include <exception>
#include <string>

#include "types/Mat.hpp"
#include "types/Vec.hpp"

namespace Zappy
{

class VRSession;

/**
 * @class VRInput
 * @brief Wraps the OpenXR action set driving VR controllers: poses, select (trigger/pinch), locomotion.
 *
 * Platform-agnostic: actions, spaces and paths are all core OpenXR concepts with opaque handles,
 * so unlike VRSession this header needs no X11/GLX isolation.
 */
class VRInput
{
  public:
    /**
     * @class VRInputException
     * @brief Raised when the action set cannot be created or attached to the session.
     */
    class VRInputException : public std::exception
    {
      public:
        explicit VRInputException(const std::string &message) : _message(message)
        {
        }

        const char *what() const noexcept override
        {
            return _message.c_str();
        }

      private:
        std::string _message; ///< Error description.
    };

    static constexpr int HandCount = 2; ///< Left (0) and right (1) hand.
    static constexpr int LeftHand = 0;  ///< Index of the left hand/controller.
    static constexpr int RightHand = 1; ///< Index of the right hand/controller.

    /**
     * @struct Pose
     * @brief A hand pose located in the session's reference space, or invalid if not tracked.
     */
    struct Pose
    {
        Vec3 position;    ///< Position relative to the session's reference space.
        Quat orientation; ///< Orientation relative to the reference space.
        bool valid;       ///< False if the runtime could not locate this pose this frame.
    };

    /**
     * @brief Binds the action set to a session (creates no OpenXR objects yet).
     * @param session VR session whose instance/session the actions are created against.
     */
    explicit VRInput(VRSession &session);

    /** @brief Destroys the action spaces and action set. */
    ~VRInput();

    VRInput(const VRInput &) = delete;
    VRInput &operator=(const VRInput &) = delete;
    VRInput(VRInput &&) = delete;
    VRInput &operator=(VRInput &&) = delete;

    /**
     * @brief Creates the action set/actions, suggests controller bindings and attaches to the session.
     * @throws VRInputException If any OpenXR call fails.
     */
    void init();

    /**
     * @brief Syncs action states for the current frame (xrSyncActions) and locates poses.
     * @param predictedDisplayTimeSeconds Time returned by VRSession::beginFrame() this frame.
     */
    void sync(double predictedDisplayTimeSeconds);

    /** @brief The hand's grip pose (for rendering a held controller model). @param hand LeftHand or RightHand. @return The pose. */
    Pose gripPose(int hand) const;

    /** @brief The hand's aim pose (the pointer ray origin/direction for picking). @param hand LeftHand or RightHand. @return The pose. */
    Pose aimPose(int hand) const;

    /** @brief Whether the hand's select action (trigger) is currently held. @param hand LeftHand or RightHand. @return True if held. */
    bool selectPressed(int hand) const;

    /** @brief Whether the hand's select action was pressed this frame (rising edge). @param hand LeftHand or RightHand. @return True on the press frame only. */
    bool selectJustPressed(int hand) const;

    /** @brief The hand's thumbstick axes, each in [-1, 1]. @param hand LeftHand or RightHand. @return The 2D stick position. */
    Vec2 thumbstick(int hand) const;

    /** @brief Whether the menu button was pressed this frame (rising edge). @return True on the press frame only. */
    bool menuJustPressed() const;

  private:
    /** @brief Creates the action set and its actions (poses/select/thumbstick/menu). */
    void createActions();

    /** @brief Suggests bindings for the baseline khr/simple_controller and oculus/touch_controller profiles. */
    void suggestBindings();

    /** @brief Creates the per-hand action spaces for the grip/aim pose actions. */
    void createActionSpaces();

    /**
     * @brief Locates a hand's action space in the session's reference space.
     * @param space Action space to locate (grip or aim).
     * @param time Predicted display time to locate at.
     * @return The located pose, invalid if the runtime couldn't track it.
     */
    Pose locate(XrSpace space, XrTime time) const;

    VRSession &_session;                        ///< Session the actions are attached to.
    XrActionSet _actionSet;                      ///< The single action set driving VR input.
    XrAction _gripPoseAction;                    ///< Grip pose action (both hands, via subaction path).
    XrAction _aimPoseAction;                     ///< Aim pose action (both hands, via subaction path).
    XrAction _selectAction;                      ///< Select action (trigger/click, both hands).
    XrAction _thumbstickAction;                  ///< Thumbstick 2-axis action (both hands).
    XrAction _menuAction;                        ///< Menu button action (hand-agnostic, bound on the left).
    std::array<XrPath, HandCount> _handPaths;    ///< "/user/hand/left" and "/user/hand/right".
    std::array<XrSpace, HandCount> _gripSpaces;  ///< Per-hand action space for _gripPoseAction.
    std::array<XrSpace, HandCount> _aimSpaces;   ///< Per-hand action space for _aimPoseAction.
    std::array<Pose, HandCount> _gripPoses;      ///< Latest located grip poses.
    std::array<Pose, HandCount> _aimPoses;       ///< Latest located aim poses.
    std::array<bool, HandCount> _selectState;    ///< Latest select state, per hand.
    std::array<bool, HandCount> _selectPrevState; ///< Select state from the previous sync(), for edge detection.
    std::array<Vec2, HandCount> _thumbstickState; ///< Latest thumbstick axes, per hand.
    bool _menuState;                              ///< Latest menu button state.
    bool _menuPrevState;                           ///< Menu button state from the previous sync().
};

} // namespace Zappy
