using System;
using UnityEngine;
using UnityEngine.InputSystem;
using Zappy.View;

namespace Zappy.VRUI
{
    /// <summary>
    /// Right-controller laser: raycasts the scene, highlights VRButtons, and fires
    /// clicks on the index trigger. Buttons take priority; otherwise tile and entity
    /// hits are reported to the session. Falls back to the mouse and main camera in
    /// the editor so everything is testable without a headset.
    /// </summary>
    public class LaserPointer : MonoBehaviour
    {
        private const float MaxDistance = 30f;

        /// <summary>Called when the trigger clicks a tile (grid coordinates).</summary>
        public Action<int, int> OnTileClicked;

        /// <summary>Called when the trigger clicks an entity.</summary>
        public Action<EntityMarker> OnEntityClicked;

        /// <summary>Called when the trigger clicks empty space.</summary>
        public Action OnMissClicked;

        /// <summary>Tile currently under the laser, or (-1,-1).</summary>
        public Vector2Int Hovered { get; private set; } = new Vector2Int(-1, -1);

        private Transform _origin;
        private LineRenderer _line;
        private VRButton _hoveredButton;

        private void Start()
        {
            var originHolder = GameObject.Find("RightControllerAnchor");
            _origin = originHolder != null ? originHolder.transform : null;

            var lineHolder = new GameObject("LaserLine");
            lineHolder.transform.SetParent(transform, false);
            _line = lineHolder.AddComponent<LineRenderer>();
            _line.startWidth = 0.004f;
            _line.endWidth = 0.002f;
            _line.positionCount = 2;
            _line.material = MaterialCache.GetUnlitTransparent(new Color(1f, 0.6f, 0.2f, 0.7f));
        }

        private void Update()
        {
            Ray ray;
            bool clicked;
            if (_origin != null && OVRInput.IsControllerConnected(OVRInput.Controller.RTouch))
            {
                ray = new Ray(_origin.position, _origin.forward);
                clicked = OVRInput.GetDown(OVRInput.Button.PrimaryIndexTrigger, OVRInput.Controller.RTouch);
                _line.enabled = true;
            }
            else if (Camera.main != null && Mouse.current != null)
            {
                // Editor / no headset: point with the mouse.
                ray = Camera.main.ScreenPointToRay(Mouse.current.position.ReadValue());
                clicked = Mouse.current.leftButton.wasPressedThisFrame;
                _line.enabled = false;
            }
            else
            {
                return;
            }

            bool hit = Physics.Raycast(ray, out RaycastHit info, MaxDistance);
            UpdateLine(ray, hit ? info.distance : MaxDistance);
            UpdateHover(hit ? info.collider : null);

            if (!clicked)
                return;
            if (_hoveredButton != null)
            {
                _hoveredButton.Click();
                return;
            }
            if (hit)
            {
                var entity = info.collider.GetComponentInParent<EntityMarker>();
                if (entity != null)
                {
                    OnEntityClicked?.Invoke(entity);
                    return;
                }
                var tile = info.collider.GetComponentInParent<TileMarker>();
                if (tile != null)
                {
                    OnTileClicked?.Invoke(tile.X, tile.Y);
                    return;
                }
            }
            OnMissClicked?.Invoke();
        }

        private void UpdateLine(Ray ray, float distance)
        {
            if (!_line.enabled)
                return;
            _line.SetPosition(0, ray.origin);
            _line.SetPosition(1, ray.origin + ray.direction * distance);
        }

        private void UpdateHover(Collider hovered)
        {
            VRButton button = hovered != null ? hovered.GetComponentInParent<VRButton>() : null;
            if (_hoveredButton != null && _hoveredButton != button)
                _hoveredButton.SetHover(false);
            _hoveredButton = button;
            if (_hoveredButton != null)
                _hoveredButton.SetHover(true);

            TileMarker tile = hovered != null ? hovered.GetComponentInParent<TileMarker>() : null;
            Hovered = tile != null ? new Vector2Int(tile.X, tile.Y) : new Vector2Int(-1, -1);
        }
    }
}
