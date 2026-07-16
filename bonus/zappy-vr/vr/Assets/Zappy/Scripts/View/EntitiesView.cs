using System.Collections.Generic;
using UnityEngine;
using Zappy.Model;

namespace Zappy.View
{
    /// <summary>
    /// Players and eggs, browser-viewer style: every trantorian is a ball whose size
    /// and color follow its level, standing on a team-colored disc (the browser's
    /// ellipse base), with a small white nose showing the orientation and a "Lv.N"
    /// label overhead. Eggs are small team-tinted spheres. A focused player dims all
    /// the others, mirroring the browser's alpha 0.18 focus mode.
    /// </summary>
    public class EntitiesView : MonoBehaviour
    {
        private const float MoveSpeed = 6f;
        private const float DimAlpha = 0.18f;

        private class PlayerVisual
        {
            public GameObject Root;
            public Transform Body; // Bobbing, rotating part: robot + label + beacon.
            public GameObject Model;
            public BoxCollider Collider;
            public Renderer DiscRenderer;
            public Renderer BeaconRenderer;
            public TextMesh Label;
            public int Level = -1;
            public bool Dimmed;
        }

        private GameState _state;
        private readonly Dictionary<int, PlayerVisual> _players = new();
        private readonly Dictionary<int, GameObject> _eggs = new();
        private long _version = -1;
        private int _povPlayer = -1;

        /// <summary>Player id currently focused, or -1: everyone fully opaque.</summary>
        public int FocusedPlayer { get; private set; } = -1;

        public void Bind(GameState state)
        {
            _state = state;
            _version = -1;
        }

        public void SetFocus(int playerId)
        {
            FocusedPlayer = playerId;
            RefreshDim();
        }

        public void ClearFocus() => SetFocus(-1);

        /// <summary>
        /// The player currently embodied in POV: its visual is hidden entirely (the
        /// user must not see the body they are inside), and focus dimming of everyone
        /// else is suspended so the world reads normally from within. -1 restores.
        /// </summary>
        public void SetPovPlayer(int playerId)
        {
            if (_povPlayer == playerId)
                return;
            if (_povPlayer >= 0 && _players.TryGetValue(_povPlayer, out PlayerVisual previous))
                previous.Root.SetActive(true);
            _povPlayer = playerId;
            if (playerId >= 0 && _players.TryGetValue(playerId, out PlayerVisual embodied))
                embodied.Root.SetActive(false);
            RefreshDim();
        }

        private bool ShouldDim(int playerId)
        {
            return _povPlayer < 0 && FocusedPlayer >= 0 && playerId != FocusedPlayer;
        }

        private void RefreshDim()
        {
            foreach (KeyValuePair<int, PlayerVisual> pair in _players)
                ApplyDim(pair.Value, ShouldDim(pair.Key));
        }

        /// <summary>The world transform of a player's visual (for speech bubbles), or null.</summary>
        public Transform PlayerTransform(int id)
        {
            return _players.TryGetValue(id, out PlayerVisual visual) ? visual.Root.transform : null;
        }

        /// <summary>
        /// Handles for gesture animations: the bobbing/turning Body (props parented
        /// here follow the robot's heading) and the Model (free to tilt — nothing
        /// else writes its rotation).
        /// </summary>
        public bool TryGetGestureRig(int id, out Transform body, out Transform model, out int level)
        {
            if (_players.TryGetValue(id, out PlayerVisual visual))
            {
                body = visual.Body;
                model = visual.Model.transform;
                level = visual.Level;
                return true;
            }
            body = null;
            model = null;
            level = 1;
            return false;
        }

        /// <summary>Robot height in map-local units for a level.</summary>
        public static float RobotHeight(int level) => LevelStyle.Diameter(level) * 1.6f;

        /// <summary>Height of a player's head top in map-local units (label/bubble anchor).</summary>
        public static float HeadHeight(int level) => 0.15f + RobotHeight(level) + 0.05f;

        private void Update()
        {
            if (_state == null)
                return;
            if (_state.Version != _version)
            {
                _version = _state.Version;
                Sync();
            }
            Animate();
        }

        private int _lastPlayerCount = -1;

        private void Sync()
        {
            if (_state.Players.Count != _lastPlayerCount)
            {
                _lastPlayerCount = _state.Players.Count;
                Debug.Log($"[Zappy] entities: state has {_lastPlayerCount} players, {_players.Count} visuals built");
            }
            foreach (KeyValuePair<int, PlayerData> pair in _state.Players)
            {
                if (!_players.TryGetValue(pair.Key, out PlayerVisual visual))
                {
                    visual = BuildPlayer(pair.Value);
                    _players[pair.Key] = visual;
                    ApplyDim(visual, ShouldDim(pair.Key));
                    if (pair.Key == _povPlayer)
                        visual.Root.SetActive(false);
                }
                if (visual.Level != pair.Value.Level)
                    ApplyLevel(visual, pair.Value.Level);
            }
            RemoveOrphans(_players, id => _state.Players.ContainsKey(id), visual => Destroy(visual.Root));

            foreach (KeyValuePair<int, EggData> pair in _state.Eggs)
            {
                if (!_eggs.ContainsKey(pair.Key))
                    _eggs[pair.Key] = BuildEgg(pair.Value);
            }
            RemoveOrphans(_eggs, id => _state.Eggs.ContainsKey(id), Destroy);

            if (_povPlayer >= 0 && !_state.Players.ContainsKey(_povPlayer))
            {
                _povPlayer = -1; // The embodied player died; its visual is gone anyway.
                RefreshDim();
            }
            if (FocusedPlayer >= 0 && !_state.Players.ContainsKey(FocusedPlayer))
                ClearFocus(); // The focused player died.
        }

        private static void RemoveOrphans<T>(Dictionary<int, T> visuals, System.Predicate<int> alive, System.Action<T> destroy)
        {
            List<int> orphans = null;
            foreach (KeyValuePair<int, T> pair in visuals)
            {
                if (!alive(pair.Key))
                    (orphans ??= new List<int>()).Add(pair.Key);
            }
            if (orphans == null)
                return;
            foreach (int id in orphans)
            {
                destroy(visuals[id]);
                visuals.Remove(id);
            }
        }

        private void Animate()
        {
            foreach (KeyValuePair<int, PlayerVisual> pair in _players)
            {
                if (!_state.Players.TryGetValue(pair.Key, out PlayerData player))
                    continue;
                PlayerVisual visual = pair.Value;
                Vector3 target = MapView.TileCenter(player.X, player.Y);
                visual.Root.transform.localPosition =
                    Vector3.Lerp(visual.Root.transform.localPosition, target, Time.deltaTime * MoveSpeed);

                // Players hover and bob — the one motion nothing static on the map has.
                // The phase is offset per player so a crowd does not bob in lockstep.
                float bob = 0.10f + 0.05f * Mathf.Sin(Time.time * 2.5f + player.Id * 1.7f);
                visual.Body.localPosition = new Vector3(0f, bob, 0f);

                // The robot itself turns to face its orientation.
                var heading = Quaternion.Euler(0f, OrientationYaw(player.Orientation), 0f);
                visual.Body.localRotation = Quaternion.Slerp(visual.Body.localRotation, heading, Time.deltaTime * MoveSpeed);
            }
        }

        private static float OrientationYaw(int orientation)
        {
            // Grid north = -Z in map space (row 0 at the back); the robot model
            // faces +Z at identity, so north means a 180° turn.
            return orientation switch
            {
                1 => 180f, // North
                2 => 270f, // East
                3 => 0f,   // South
                4 => 90f,  // West
                _ => 180f
            };
        }

        private PlayerVisual BuildPlayer(PlayerData player)
        {
            var root = new GameObject($"Player_{player.Id}");
            root.transform.SetParent(transform, false);
            root.transform.localPosition = MapView.TileCenter(player.X, player.Y);
            var marker = root.AddComponent<EntityMarker>();
            marker.Type = "player";
            marker.Number = player.Id;

            // Everything that hovers/bobs/turns lives under Body; the disc stays grounded.
            var body = new GameObject("Body");
            body.transform.SetParent(root.transform, false);

            // The robot mesh, normalized to 1 unit tall; ApplyLevel scales it.
            GameObject model = ModelLibrary.Spawn("Robot", body.transform, LevelStyle.ColorOf(player.Level));
            // A simple box carries the marker lookup for entity picking.
            var pickBox = root.AddComponent<BoxCollider>();

            GameObject disc = GameObject.CreatePrimitive(PrimitiveType.Cylinder);
            disc.name = "TeamDisc";
            Destroy(disc.GetComponent<Collider>());
            disc.transform.SetParent(root.transform, false);
            disc.transform.localPosition = new Vector3(0f, 0.055f, 0f);
            disc.GetComponent<Renderer>().sharedMaterial = MaterialCache.GetLit(TeamColors.Get(player.Team));

            GameObject beacon = GameObject.CreatePrimitive(PrimitiveType.Cylinder);
            beacon.name = "Beacon";
            Destroy(beacon.GetComponent<Collider>());
            beacon.transform.SetParent(body.transform, false);
            Color beaconColor = TeamColors.Get(player.Team);
            beaconColor.a = 0.55f;
            beacon.GetComponent<Renderer>().sharedMaterial = MaterialCache.GetUnlitTransparent(beaconColor);

            var labelHolder = new GameObject("Level");
            labelHolder.transform.SetParent(body.transform, false);
            labelHolder.AddComponent<Billboard>();
            var label = labelHolder.AddComponent<TextMesh>();
            label.font = UiFontProvider.Font;
            labelHolder.GetComponent<MeshRenderer>().material = UiFontProvider.Font.material;
            label.fontSize = 48;
            label.characterSize = 0.12f * 10f / label.fontSize;
            label.anchor = TextAnchor.LowerCenter;
            label.alignment = TextAlignment.Center;
            label.color = Color.white;

            var visual = new PlayerVisual
            {
                Root = root,
                Body = body.transform,
                Model = model,
                Collider = pickBox,
                DiscRenderer = disc.GetComponent<Renderer>(),
                BeaconRenderer = beacon.GetComponent<Renderer>(),
                Label = label
            };
            ApplyLevel(visual, player.Level);
            return visual;
        }

        /// <summary>Ball size, ball color, disc size and label all follow the level.</summary>
        private void ApplyLevel(PlayerVisual visual, int level)
        {
            visual.Level = level;
            float diameter = LevelStyle.Diameter(level);
            float height = RobotHeight(level);
            visual.Model.transform.localScale = Vector3.one * height;
            visual.Collider.center = new Vector3(0f, 0.12f + height / 2f, 0f);
            visual.Collider.size = new Vector3(height * 0.6f, height, height * 0.6f);
            visual.DiscRenderer.transform.localScale = new Vector3(diameter + 0.14f, 0.01f, diameter + 0.14f);
            // Thin light pillar above the ball (cylinder height = 2 * y scale).
            visual.BeaconRenderer.transform.localScale = new Vector3(0.05f, 0.25f, 0.05f);
            visual.BeaconRenderer.transform.localPosition = new Vector3(0f, HeadHeight(level) + 0.30f, 0f);
            visual.Label.text = $"Lv.{level}";
            visual.Label.transform.localPosition = new Vector3(0f, HeadHeight(level), 0f);
            ApplyDim(visual, visual.Dimmed);
        }

        private void ApplyDim(PlayerVisual visual, bool dimmed)
        {
            visual.Dimmed = dimmed;
            Color levelColor = LevelStyle.ColorOf(visual.Level);
            PlayerData player = _state.Players.TryGetValue(GetId(visual), out PlayerData data) ? data : null;
            Color teamColor = TeamColors.Get(player?.Team);
            Color beaconColor = teamColor;
            if (dimmed)
            {
                levelColor.a = DimAlpha;
                teamColor.a = DimAlpha;
                beaconColor.a = DimAlpha * 0.5f;
                ModelLibrary.TintTransparent(visual.Model, levelColor);
                visual.DiscRenderer.sharedMaterial = MaterialCache.GetUnlitTransparent(teamColor);
                visual.Label.color = new Color(1f, 1f, 1f, DimAlpha);
            }
            else
            {
                beaconColor.a = 0.55f;
                ModelLibrary.Tint(visual.Model, levelColor);
                visual.DiscRenderer.sharedMaterial = MaterialCache.GetLit(teamColor);
                visual.Label.color = Color.white;
            }
            visual.BeaconRenderer.sharedMaterial = MaterialCache.GetUnlitTransparent(beaconColor);
        }

        private static int GetId(PlayerVisual visual)
        {
            return visual.Root.GetComponent<EntityMarker>().Number;
        }

        private GameObject BuildEgg(EggData egg)
        {
            var root = new GameObject($"Egg_{egg.Id}");
            root.transform.SetParent(transform, false);
            root.transform.localPosition = MapView.TileCenter(egg.X, egg.Y) + new Vector3(0f, 0.12f, 0f);
            var marker = root.AddComponent<EntityMarker>();
            marker.Type = "egg";
            marker.Number = egg.Id;

            GameObject shell = GameObject.CreatePrimitive(PrimitiveType.Sphere);
            shell.name = "Shell";
            shell.transform.SetParent(root.transform, false);
            shell.transform.localScale = new Vector3(0.13f, 0.17f, 0.13f);
            string team = _state.Players.TryGetValue(egg.PlayerId, out PlayerData father) ? father.Team : null;
            Color tint = Color.Lerp(Color.white, TeamColors.Get(team), 0.45f);
            shell.GetComponent<Renderer>().sharedMaterial = MaterialCache.GetLit(tint);
            return root;
        }
    }

    /// <summary>The one legacy font every TextMesh in the app shares.</summary>
    public static class UiFontProvider
    {
        private static Font _font;

        public static Font Font => _font != null ? _font : _font = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
    }
}
