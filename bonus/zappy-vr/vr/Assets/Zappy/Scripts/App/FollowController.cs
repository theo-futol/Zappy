using UnityEngine;
using Zappy.Model;
using Zappy.View;

namespace Zappy.App
{
    /// <summary>
    /// VR version of the browser's camera-lock: free mode frames the whole map as a
    /// tabletop at the anchor; tracking a player slides the map so that player stays
    /// centered. Moving around is the user's job (PlayerLocomotion) — this component
    /// never moves the map under a navigating user.
    /// </summary>
    public class FollowController : MonoBehaviour
    {
        private const float TabletopSpan = 1.6f;  // Widest map dimension in meters on the table.
        private const float MaxTileSize = 0.14f;  // Cap so small maps do not become huge tiles.
        private const float PovTileSize = 0.6f;   // Tile size in POV mode (approach of life-size).
        private const float MoveLerp = 2.5f;

        private GameState _state;
        private Transform _mapRoot;
        private Vector3 _anchor;
        private Vector3 _povEyePoint;
        private float _povYaw;
        private bool _framed;

        public int TrackedPlayer { get; private set; } = -1;

        public void Bind(GameState state, Transform mapRoot, Vector3 anchor)
        {
            _state = state;
            _mapRoot = mapRoot;
            _anchor = anchor;
            _framed = false;
            TrackedPlayer = -1;
        }

        public void SetAnchor(Vector3 anchor)
        {
            _anchor = anchor;
            _framed = false; // Snap to the new spot instead of drifting across the room.
        }

        /// <summary>True while the user sees the world from the tracked trantorian.</summary>
        public bool Pov { get; private set; }

        public void Track(int playerId)
        {
            TrackedPlayer = playerId;
            Pov = false;
        }

        public void TogglePov()
        {
            if (TrackedPlayer < 0)
                return;
            if (Pov)
            {
                Pov = false;
                return;
            }
            // Freeze the eye anchor where the user's head is right now. The head
            // stays free afterwards: looking or leaning explores around the
            // trantorian instead of dragging the world along with the gaze.
            Camera head = Camera.main;
            if (head == null)
                return;
            _povEyePoint = head.transform.position - new Vector3(0f, 0.15f, 0f);
            _povYaw = head.transform.eulerAngles.y;
            Pov = true;
        }

        public void Detach()
        {
            TrackedPlayer = -1;
            Pov = false;
        }

        private void Update()
        {
            if (_state == null || _mapRoot == null || _state.Width == 0)
                return;

            if (TrackedPlayer >= 0 && !_state.Players.ContainsKey(TrackedPlayer))
                Detach(); // The tracked player died.

            float scale = TabletopScale();
            if (TrackedPlayer >= 0 && Pov)
            {
                UpdatePov(_state.Players[TrackedPlayer]);
            }
            else if (TrackedPlayer >= 0)
            {
                PlayerData player = _state.Players[TrackedPlayer];
                MoveRootSo(MapView.TileCenter(player.X, player.Y), _anchor, scale, Quaternion.identity, false);
            }
            else
            {
                var center = new Vector3((_state.Width - 1) / 2f, 0f, (_state.Height - 1) / 2f);
                MoveRootSo(center, _anchor, scale, Quaternion.identity, !_framed);
                _framed = true;
            }
        }

        /// <summary>
        /// Grows the map toward life-size and pins the tracked trantorian's eyes to
        /// the anchor captured when POV started, its heading mapped onto the user's
        /// forward at that moment. The world only moves when the trantorian does;
        /// the user's own head looks around it freely.
        /// </summary>
        private void UpdatePov(PlayerData player)
        {
            // The robot hovers ~0.10 above the tile; its eyes sit near the model top.
            float eyeHeight = 0.25f + EntitiesView.RobotHeight(player.Level) * 0.9f;
            Vector3 eyeLocal = MapView.TileCenter(player.X, player.Y) + new Vector3(0f, eyeHeight, 0f);

            // Same orientation mapping as EntitiesView.OrientationYaw.
            float entityYaw = player.Orientation switch
            {
                1 => 180f, // North
                2 => 270f, // East
                3 => 0f,   // South
                4 => 90f,  // West
                _ => 180f
            };
            var rotation = Quaternion.Euler(0f, _povYaw - entityYaw, 0f);
            MoveRootSo(eyeLocal, _povEyePoint, PovTileSize, rotation, false);
        }

        private float TabletopScale()
        {
            int span = Mathf.Max(_state.Width, _state.Height);
            return Mathf.Min(MaxTileSize, TabletopSpan / span);
        }

        /// <summary>Root pose that puts a given map-local point at a given world point.</summary>
        private void MoveRootSo(Vector3 mapLocalPoint, Vector3 worldPoint, float scale, Quaternion rotation, bool snap)
        {
            Vector3 position = worldPoint - rotation * (mapLocalPoint * scale);
            Vector3 targetScale = Vector3.one * scale;
            if (snap)
            {
                _mapRoot.SetPositionAndRotation(position, rotation);
                _mapRoot.localScale = targetScale;
                return;
            }
            float t = Time.deltaTime * MoveLerp;
            _mapRoot.position = Vector3.Lerp(_mapRoot.position, position, t);
            _mapRoot.rotation = Quaternion.Slerp(_mapRoot.rotation, rotation, t);
            _mapRoot.localScale = Vector3.Lerp(_mapRoot.localScale, targetScale, t);
        }
    }
}
