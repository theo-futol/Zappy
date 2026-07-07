#include "VR/handtracking/VRHandTracking.hpp"

#include <cmath>

#include "VR/session/VRSession.hpp"

namespace Zappy
{

VRHandTracking::VRHandTracking(VRSession &session)
    : _session(session), _createHandTracker(nullptr), _destroyHandTracker(nullptr),
      _locateHandJoints(nullptr), _trackers{XR_NULL_HANDLE, XR_NULL_HANDLE}, _pinching{false, false}, _pinchingPrev{false, false}
{
}

VRHandTracking::~VRHandTracking()
{
    for (XrHandTrackerEXT tracker : _trackers)
        if (tracker != XR_NULL_HANDLE && _destroyHandTracker != nullptr)
            _destroyHandTracker(tracker);
}

void VRHandTracking::init()
{
    if (!_session.extensionEnabled(XR_EXT_HAND_TRACKING_EXTENSION_NAME))
        throw VRHandTrackingException("XR_EXT_hand_tracking was not enabled by the runtime");

    XrInstance instance = _session.instanceHandle();

    if (XR_FAILED(xrGetInstanceProcAddr(instance, "xrCreateHandTrackerEXT", reinterpret_cast<PFN_xrVoidFunction *>(&_createHandTracker))) ||
        XR_FAILED(xrGetInstanceProcAddr(instance, "xrDestroyHandTrackerEXT", reinterpret_cast<PFN_xrVoidFunction *>(&_destroyHandTracker))) ||
        XR_FAILED(xrGetInstanceProcAddr(instance, "xrLocateHandJointsEXT", reinterpret_cast<PFN_xrVoidFunction *>(&_locateHandJoints))))
        throw VRHandTrackingException("Failed to resolve XR_EXT_hand_tracking functions");

    std::array<XrHandEXT, HandCount> hands{XR_HAND_LEFT_EXT, XR_HAND_RIGHT_EXT};

    for (int hand = 0; hand < HandCount; ++hand)
    {
        std::size_t index = static_cast<std::size_t>(hand);
        XrHandTrackerCreateInfoEXT createInfo{};

        createInfo.type = XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT;
        createInfo.hand = hands[index];
        createInfo.handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT;
        if (XR_FAILED(_createHandTracker(_session.sessionHandle(), &createInfo, &_trackers[index])))
            throw VRHandTrackingException("xrCreateHandTrackerEXT failed for one hand");
    }
}

void VRHandTracking::locate(double predictedDisplayTimeSeconds)
{
    XrTime time = static_cast<XrTime>(predictedDisplayTimeSeconds * 1e9);
    constexpr XrSpaceLocationFlags TrackedBits = XR_SPACE_LOCATION_POSITION_VALID_BIT;

    for (int hand = 0; hand < HandCount; ++hand)
    {
        std::size_t index = static_cast<std::size_t>(hand);
        std::array<XrHandJointLocationEXT, XR_HAND_JOINT_COUNT_EXT> joints{};
        XrHandJointLocationsEXT locations{};

        locations.type = XR_TYPE_HAND_JOINT_LOCATIONS_EXT;
        locations.jointCount = static_cast<uint32_t>(joints.size());
        locations.jointLocations = joints.data();

        XrHandJointsLocateInfoEXT locateInfo{};

        locateInfo.type = XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT;
        locateInfo.baseSpace = _session.spaceHandle();
        locateInfo.time = time;

        _pinchingPrev[index] = _pinching[index];
        _pinching[index] = false;
        if (XR_FAILED(_locateHandJoints(_trackers[index], &locateInfo, &locations)) || locations.isActive == XR_FALSE)
            continue;

        const XrHandJointLocationEXT &thumbTip = joints[XR_HAND_JOINT_THUMB_TIP_EXT];
        const XrHandJointLocationEXT &indexTip = joints[XR_HAND_JOINT_INDEX_TIP_EXT];

        if ((thumbTip.locationFlags & TrackedBits) == 0 || (indexTip.locationFlags & TrackedBits) == 0)
            continue;

        float dx = thumbTip.pose.position.x - indexTip.pose.position.x;
        float dy = thumbTip.pose.position.y - indexTip.pose.position.y;
        float dz = thumbTip.pose.position.z - indexTip.pose.position.z;
        float distance = std::sqrt(dx * dx + dy * dy + dz * dz);

        _pinching[index] = distance < PinchThresholdMeters;
    }
}

bool VRHandTracking::isPinching(int hand) const
{
    return _pinching[static_cast<std::size_t>(hand)];
}

bool VRHandTracking::pinchJustStarted(int hand) const
{
    std::size_t index = static_cast<std::size_t>(hand);

    return _pinching[index] && !_pinchingPrev[index];
}

} // namespace Zappy
