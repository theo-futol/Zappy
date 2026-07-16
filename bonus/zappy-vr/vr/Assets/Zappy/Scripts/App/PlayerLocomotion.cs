using UnityEngine;
using UnityEngine.InputSystem;

namespace Zappy.App
{
    /// <summary>
    /// First-person navigation over the tabletop, on either thumbstick:
    ///  - stick up/down   : move forward/back along where you look
    ///  - stick left/right: turn yourself (the map never moves)
    ///  - stick click     : cycle the POV height in fixed steps
    ///                      (below the map, map level, a bit above, bird's-eye)
    /// Moves the OVR camera rig — in VR the head is the camera.
    /// Editor fallback: W/S move, A/D turn, Space cycles the height.
    /// </summary>
    public class PlayerLocomotion : MonoBehaviour
    {
        private const float MoveSpeed = 1.5f;   // m/s at full stick.
        private const float TurnSpeed = 60f;    // deg/s at full stick.
        private const float Deadzone = 0.15f;
        private const float RangeLimit = 8f;    // Max distance from the start pose.

        // Eye height relative to the tabletop plane: below the map, same height,
        // a few heads taller, high above the map (bird's-eye).
        private static readonly float[] EyeAboveTable = { -0.35f, 0.15f, 0.9f, 2.0f };

        private Transform _rig;
        private Vector3 _homePosition;
        private Quaternion _homeRotation;
        private float _tableHeight = 0.95f;
        private int _heightIndex = -1; // Unset until the first stick click.

        /// <summary>World height of the tabletop plane the steps are relative to.</summary>
        public void SetTableHeight(float height) => _tableHeight = height;

        /// <summary>True while the sticks (or editor keys) are driving the user.</summary>
        public bool IsMoving { get; private set; }

        /// <summary>
        /// Back to the session's start pose — in front of the map, facing it — with
        /// the eye placed slightly above the tabletop plane.
        /// </summary>
        public void ResetView()
        {
            if (_rig == null)
                return;
            _rig.SetPositionAndRotation(_homePosition, _homeRotation);
            float eyeAboveRig = Camera.main != null ? Camera.main.transform.position.y - _rig.position.y : 1.5f;
            Vector3 position = _rig.position;
            position.y = _tableHeight + 0.45f - eyeAboveRig;
            _rig.position = position;
            _heightIndex = -1; // Height stepping restarts from the first step.
        }

        private void Start()
        {
            OVRCameraRig rig = FindFirstObjectByType<OVRCameraRig>();
            if (rig == null)
            {
                Debug.LogWarning("[Zappy] No OVRCameraRig found; locomotion disabled");
                return;
            }
            _rig = rig.transform;
            _homePosition = _rig.position;
            _homeRotation = _rig.rotation;
        }

        private void Update()
        {
            if (_rig == null)
                return;

            Vector2 stick = ApplyDeadzone(OVRInput.Get(OVRInput.Axis2D.PrimaryThumbstick, OVRInput.Controller.LTouch))
                            + ApplyDeadzone(OVRInput.Get(OVRInput.Axis2D.PrimaryThumbstick, OVRInput.Controller.RTouch));
            bool cycleHeight = OVRInput.GetDown(OVRInput.Button.PrimaryThumbstick, OVRInput.Controller.LTouch)
                               || OVRInput.GetDown(OVRInput.Button.PrimaryThumbstick, OVRInput.Controller.RTouch);

            Keyboard keyboard = Keyboard.current;
            if (keyboard != null)
            {
                stick.x += (keyboard.dKey.isPressed ? 1f : 0f) - (keyboard.aKey.isPressed ? 1f : 0f);
                stick.y += (keyboard.wKey.isPressed ? 1f : 0f) - (keyboard.sKey.isPressed ? 1f : 0f);
                cycleHeight |= keyboard.spaceKey.wasPressedThisFrame;
            }

            IsMoving = stick != Vector2.zero;

            Vector3 pivot = Camera.main != null ? Camera.main.transform.position : _rig.position;

            // Left/right turns the user in place (around the head, so the view
            // pivots instead of orbiting).
            if (stick.x != 0f)
                _rig.RotateAround(pivot, Vector3.up, stick.x * TurnSpeed * Time.deltaTime);

            // Up/down moves along the horizontal view direction.
            if (stick.y != 0f)
            {
                Vector3 forward = Vector3.forward;
                Camera head = Camera.main;
                if (head != null)
                {
                    forward = head.transform.forward;
                    forward.y = 0f;
                    forward = forward.sqrMagnitude > 0.001f ? forward.normalized : Vector3.forward;
                }
                Vector3 position = _rig.position + forward * (stick.y * MoveSpeed * Time.deltaTime);
                Vector3 horizontal = Vector3.ClampMagnitude(
                    new Vector3(position.x - _homePosition.x, 0f, position.z - _homePosition.z), RangeLimit);
                _rig.position = new Vector3(_homePosition.x + horizontal.x, position.y, _homePosition.z + horizontal.z);
            }

            if (cycleHeight)
            {
                _heightIndex = (_heightIndex + 1) % EyeAboveTable.Length;
                // Place the EYE at the requested height: shift the rig by however
                // far the eye currently sits above the rig floor.
                float eyeAboveRig = Camera.main != null ? Camera.main.transform.position.y - _rig.position.y : 1.5f;
                Vector3 position = _rig.position;
                position.y = _tableHeight + EyeAboveTable[_heightIndex] - eyeAboveRig;
                _rig.position = position;
            }
        }

        private static Vector2 ApplyDeadzone(Vector2 stick)
        {
            return stick.magnitude < Deadzone ? Vector2.zero : stick;
        }

        private void OnDisable()
        {
            IsMoving = false;
        }

        private void OnDestroy()
        {
            // Session over: land the user back where they started so the menu
            // never appears floating in the void.
            if (_rig != null)
                _rig.SetPositionAndRotation(_homePosition, _homeRotation);
        }
    }
}
