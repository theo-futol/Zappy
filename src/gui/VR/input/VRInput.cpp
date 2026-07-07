#include "VR/input/VRInput.hpp"

#include <cstring>

#include "VR/session/VRSession.hpp"

namespace Zappy
{

namespace
{

XrPath stringToPath(XrInstance instance, const char *pathString)
{
    XrPath path = XR_NULL_PATH;

    xrStringToPath(instance, pathString, &path);
    return path;
}

XrAction createAction(XrActionSet actionSet, const char *name, XrActionType type, const std::array<XrPath, VRInput::HandCount> *subactionPaths)
{
    XrActionCreateInfo createInfo{};

    createInfo.type = XR_TYPE_ACTION_CREATE_INFO;
    createInfo.actionType = type;
    std::strncpy(createInfo.actionName, name, XR_MAX_ACTION_NAME_SIZE - 1);
    std::strncpy(createInfo.localizedActionName, name, XR_MAX_LOCALIZED_ACTION_NAME_SIZE - 1);
    if (subactionPaths != nullptr)
    {
        createInfo.countSubactionPaths = static_cast<uint32_t>(subactionPaths->size());
        createInfo.subactionPaths = subactionPaths->data();
    }

    XrAction action = XR_NULL_HANDLE;

    xrCreateAction(actionSet, &createInfo, &action);
    return action;
}

} // namespace

VRInput::VRInput(VRSession &session)
    : _session(session), _actionSet(XR_NULL_HANDLE), _gripPoseAction(XR_NULL_HANDLE), _aimPoseAction(XR_NULL_HANDLE), _selectAction(XR_NULL_HANDLE),
      _thumbstickAction(XR_NULL_HANDLE), _menuAction(XR_NULL_HANDLE), _handPaths{XR_NULL_PATH, XR_NULL_PATH}, _gripSpaces{XR_NULL_HANDLE, XR_NULL_HANDLE},
      _aimSpaces{XR_NULL_HANDLE, XR_NULL_HANDLE}, _gripPoses{}, _aimPoses{}, _selectState{false, false}, _selectPrevState{false, false}, _thumbstickState{}, _menuState(false),
      _menuPrevState(false)
{
}

VRInput::~VRInput()
{
    for (XrSpace space : _gripSpaces)
        if (space != XR_NULL_HANDLE)
            xrDestroySpace(space);
    for (XrSpace space : _aimSpaces)
        if (space != XR_NULL_HANDLE)
            xrDestroySpace(space);
    if (_actionSet != XR_NULL_HANDLE)
        xrDestroyActionSet(_actionSet);
}

void VRInput::init()
{
    XrInstance instance = _session.instanceHandle();

    _handPaths[LeftHand] = stringToPath(instance, "/user/hand/left");
    _handPaths[RightHand] = stringToPath(instance, "/user/hand/right");

    XrActionSetCreateInfo setCreateInfo{};

    setCreateInfo.type = XR_TYPE_ACTION_SET_CREATE_INFO;
    std::strncpy(setCreateInfo.actionSetName, "gameplay", XR_MAX_ACTION_SET_NAME_SIZE - 1);
    std::strncpy(setCreateInfo.localizedActionSetName, "Gameplay", XR_MAX_LOCALIZED_ACTION_SET_NAME_SIZE - 1);
    setCreateInfo.priority = 0;
    if (XR_FAILED(xrCreateActionSet(instance, &setCreateInfo, &_actionSet)))
        throw VRInputException("xrCreateActionSet failed");

    createActions();
    suggestBindings();

    XrSessionActionSetsAttachInfo attachInfo{};

    attachInfo.type = XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO;
    attachInfo.countActionSets = 1;
    attachInfo.actionSets = &_actionSet;
    if (XR_FAILED(xrAttachSessionActionSets(_session.sessionHandle(), &attachInfo)))
        throw VRInputException("xrAttachSessionActionSets failed");

    createActionSpaces();
}

void VRInput::createActions()
{
    _gripPoseAction = createAction(_actionSet, "grip_pose", XR_ACTION_TYPE_POSE_INPUT, &_handPaths);
    _aimPoseAction = createAction(_actionSet, "aim_pose", XR_ACTION_TYPE_POSE_INPUT, &_handPaths);
    _selectAction = createAction(_actionSet, "select", XR_ACTION_TYPE_BOOLEAN_INPUT, &_handPaths);
    _thumbstickAction = createAction(_actionSet, "thumbstick", XR_ACTION_TYPE_VECTOR2F_INPUT, &_handPaths);
    _menuAction = createAction(_actionSet, "menu", XR_ACTION_TYPE_BOOLEAN_INPUT, nullptr);

    if (_gripPoseAction == XR_NULL_HANDLE || _aimPoseAction == XR_NULL_HANDLE || _selectAction == XR_NULL_HANDLE || _thumbstickAction == XR_NULL_HANDLE ||
        _menuAction == XR_NULL_HANDLE)
        throw VRInputException("xrCreateAction failed for one or more VR input actions");
}

void VRInput::suggestBindings()
{
    XrInstance instance = _session.instanceHandle();

    // Baseline profile every conformant OpenXR runtime supports: poses, a boolean select, and a
    // menu button, but no thumbstick.
    {
        std::array<XrActionSuggestedBinding, 7> bindings{{
            {_gripPoseAction, stringToPath(instance, "/user/hand/left/input/grip/pose")},
            {_gripPoseAction, stringToPath(instance, "/user/hand/right/input/grip/pose")},
            {_aimPoseAction, stringToPath(instance, "/user/hand/left/input/aim/pose")},
            {_aimPoseAction, stringToPath(instance, "/user/hand/right/input/aim/pose")},
            {_selectAction, stringToPath(instance, "/user/hand/left/input/select/click")},
            {_selectAction, stringToPath(instance, "/user/hand/right/input/select/click")},
            {_menuAction, stringToPath(instance, "/user/hand/left/input/menu/click")},
        }};
        XrInteractionProfileSuggestedBinding suggestion{};

        suggestion.type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING;
        suggestion.interactionProfile = stringToPath(instance, "/interaction_profiles/khr/simple_controller");
        suggestion.countSuggestedBindings = static_cast<uint32_t>(bindings.size());
        suggestion.suggestedBindings = bindings.data();
        xrSuggestInteractionProfileBindings(instance, &suggestion);
    }

    // Common real-world controllers (Meta Quest/Rift Touch): adds a real trigger and thumbstick.
    {
        std::array<XrActionSuggestedBinding, 8> bindings{{
            {_gripPoseAction, stringToPath(instance, "/user/hand/left/input/grip/pose")},
            {_gripPoseAction, stringToPath(instance, "/user/hand/right/input/grip/pose")},
            {_aimPoseAction, stringToPath(instance, "/user/hand/left/input/aim/pose")},
            {_aimPoseAction, stringToPath(instance, "/user/hand/right/input/aim/pose")},
            {_selectAction, stringToPath(instance, "/user/hand/left/input/trigger/value")},
            {_selectAction, stringToPath(instance, "/user/hand/right/input/trigger/value")},
            {_thumbstickAction, stringToPath(instance, "/user/hand/left/input/thumbstick")},
            {_thumbstickAction, stringToPath(instance, "/user/hand/right/input/thumbstick")},
        }};
        XrInteractionProfileSuggestedBinding suggestion{};

        suggestion.type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING;
        suggestion.interactionProfile = stringToPath(instance, "/interaction_profiles/oculus/touch_controller");
        suggestion.countSuggestedBindings = static_cast<uint32_t>(bindings.size());
        suggestion.suggestedBindings = bindings.data();
        xrSuggestInteractionProfileBindings(instance, &suggestion);
    }
}

void VRInput::createActionSpaces()
{
    for (int hand = 0; hand < HandCount; ++hand)
    {
        XrActionSpaceCreateInfo gripInfo{};

        gripInfo.type = XR_TYPE_ACTION_SPACE_CREATE_INFO;
        gripInfo.action = _gripPoseAction;
        gripInfo.subactionPath = _handPaths[static_cast<std::size_t>(hand)];
        gripInfo.poseInActionSpace.orientation = XrQuaternionf{0.0f, 0.0f, 0.0f, 1.0f};
        if (XR_FAILED(xrCreateActionSpace(_session.sessionHandle(), &gripInfo, &_gripSpaces[static_cast<std::size_t>(hand)])))
            throw VRInputException("xrCreateActionSpace failed for a grip pose");

        XrActionSpaceCreateInfo aimInfo{};

        aimInfo.type = XR_TYPE_ACTION_SPACE_CREATE_INFO;
        aimInfo.action = _aimPoseAction;
        aimInfo.subactionPath = _handPaths[static_cast<std::size_t>(hand)];
        aimInfo.poseInActionSpace.orientation = XrQuaternionf{0.0f, 0.0f, 0.0f, 1.0f};
        if (XR_FAILED(xrCreateActionSpace(_session.sessionHandle(), &aimInfo, &_aimSpaces[static_cast<std::size_t>(hand)])))
            throw VRInputException("xrCreateActionSpace failed for an aim pose");
    }
}

VRInput::Pose VRInput::locate(XrSpace space, XrTime time) const
{
    XrSpaceLocation location{};

    location.type = XR_TYPE_SPACE_LOCATION;

    Pose pose{Vec3(0.0f), Quat(1.0f, 0.0f, 0.0f, 0.0f), false};
    constexpr XrSpaceLocationFlags TrackedBits = XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;

    if (XR_FAILED(xrLocateSpace(space, _session.spaceHandle(), time, &location)) || (location.locationFlags & TrackedBits) != TrackedBits)
        return pose;

    pose.position = Vec3(location.pose.position.x, location.pose.position.y, location.pose.position.z);
    pose.orientation = Quat(location.pose.orientation.w, location.pose.orientation.x, location.pose.orientation.y, location.pose.orientation.z);
    pose.valid = true;
    return pose;
}

void VRInput::sync(double predictedDisplayTimeSeconds)
{
    XrActiveActionSet activeSet{_actionSet, XR_NULL_PATH};
    XrActionsSyncInfo syncInfo{};

    syncInfo.type = XR_TYPE_ACTIONS_SYNC_INFO;
    syncInfo.countActiveActionSets = 1;
    syncInfo.activeActionSets = &activeSet;
    xrSyncActions(_session.sessionHandle(), &syncInfo);

    XrTime time = static_cast<XrTime>(predictedDisplayTimeSeconds * 1e9);

    for (int hand = 0; hand < HandCount; ++hand)
    {
        std::size_t index = static_cast<std::size_t>(hand);

        _gripPoses[index] = locate(_gripSpaces[index], time);
        _aimPoses[index] = locate(_aimSpaces[index], time);

        XrActionStateGetInfo getInfo{};

        getInfo.type = XR_TYPE_ACTION_STATE_GET_INFO;
        getInfo.subactionPath = _handPaths[index];

        XrActionStateBoolean selectState{};

        selectState.type = XR_TYPE_ACTION_STATE_BOOLEAN;
        getInfo.action = _selectAction;
        xrGetActionStateBoolean(_session.sessionHandle(), &getInfo, &selectState);
        _selectPrevState[index] = _selectState[index];
        _selectState[index] = selectState.isActive != XR_FALSE && selectState.currentState != XR_FALSE;

        XrActionStateVector2f stickState{};

        stickState.type = XR_TYPE_ACTION_STATE_VECTOR2F;
        getInfo.action = _thumbstickAction;
        xrGetActionStateVector2f(_session.sessionHandle(), &getInfo, &stickState);
        _thumbstickState[index] = stickState.isActive != XR_FALSE ? Vec2(stickState.currentState.x, stickState.currentState.y) : Vec2(0.0f, 0.0f);
    }

    XrActionStateGetInfo menuGetInfo{};

    menuGetInfo.type = XR_TYPE_ACTION_STATE_GET_INFO;
    menuGetInfo.action = _menuAction;

    XrActionStateBoolean menuState{};

    menuState.type = XR_TYPE_ACTION_STATE_BOOLEAN;
    xrGetActionStateBoolean(_session.sessionHandle(), &menuGetInfo, &menuState);
    _menuPrevState = _menuState;
    _menuState = menuState.isActive != XR_FALSE && menuState.currentState != XR_FALSE;
}

VRInput::Pose VRInput::gripPose(int hand) const
{
    return _gripPoses[static_cast<std::size_t>(hand)];
}

VRInput::Pose VRInput::aimPose(int hand) const
{
    return _aimPoses[static_cast<std::size_t>(hand)];
}

bool VRInput::selectPressed(int hand) const
{
    return _selectState[static_cast<std::size_t>(hand)];
}

bool VRInput::selectJustPressed(int hand) const
{
    std::size_t index = static_cast<std::size_t>(hand);

    return _selectState[index] && !_selectPrevState[index];
}

Vec2 VRInput::thumbstick(int hand) const
{
    return _thumbstickState[static_cast<std::size_t>(hand)];
}

bool VRInput::menuJustPressed() const
{
    return _menuState && !_menuPrevState;
}

} // namespace Zappy
