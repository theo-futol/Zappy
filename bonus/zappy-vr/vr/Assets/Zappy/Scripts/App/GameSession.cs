using System;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.InputSystem;
using Zappy.Model;
using Zappy.Net;
using Zappy.Protocol;
using Zappy.View;
using Zappy.VRUI;

namespace Zappy.App
{
    /// <summary>
    /// One connected viewer session: owns the bridge client, the mirrored state, the
    /// world views and the HUD, and routes laser clicks to the browser-style
    /// interactions (tile info, player focus + follow). Ends when the user clicks
    /// MENU or the bridge drops.
    /// </summary>
    public class GameSession : MonoBehaviour
    {
        private const int GameServerPort = 4242; // The manual client talks to zappy_server, not the bridge.
        private static readonly Vector3 MapAnchor = new Vector3(0f, 0.95f, 1.6f);
        // Just behind and above the table's far edge, so the HUD shares the user's
        // view of the map instead of floating off to the sides.
        private static readonly Vector3 HudAnchor = new Vector3(0f, 1.42f, 2.35f);
        private const int MaxEventsPerFrame = 256;

        /// <summary>Called once when the session ends; the argument is an error line or null.</summary>
        public Action<string> OnEnded;

        private BridgeClient _bridge;
        private GameState _state;
        private EventDispatcher _dispatcher;
        private MapView _map;
        private EntitiesView _entities;
        private HudController _hud;
        private FollowController _follow;
        private PlayerLocomotion _locomotion;
        private ComfortVignette _vignette;
        private BroadcastVoice _voice;
        private LaserPointer _laser;
        private bool _povActive;

        // The manually driven trantorian (one at a time), controller-driven:
        // stick = move/turn, B = eat, X = leave.
        private PlayerClient _manual;
        private string _manualTeam;
        private int _manualPlayerId = -1;
        private bool _manualBusy;
        private readonly HashSet<int> _idsBeforeJoin = new();
        private string _host;
        private Transform _hudRoot;
        private readonly HashSet<string> _seenEventTypes = new();
        private bool _wasConnected;
        private bool _ended;

        public void Begin(string host, int port, LaserPointer laser)
        {
            _state = new GameState();
            _dispatcher = new EventDispatcher(_state);
            _bridge = new BridgeClient();
            _bridge.Connect(host, port);
            _laser = laser;
            _host = host;

            // Anchor the tabletop and HUD in front of wherever the user actually is;
            // the world origin is wherever the runtime last recentered, not the player.
            (Vector3 mapAnchor, Vector3 hudAnchor, Quaternion userYaw) = UserFrame();

            var mapRoot = new GameObject("MapRoot");
            mapRoot.transform.SetParent(transform, false);
            _map = mapRoot.AddComponent<MapView>();
            _map.Bind(_state);
            var entities = new GameObject("Entities");
            entities.transform.SetParent(mapRoot.transform, false);
            _entities = entities.AddComponent<EntitiesView>();
            _entities.Bind(_state);
            var effects = new GameObject("Effects");
            effects.transform.SetParent(mapRoot.transform, false);
            effects.AddComponent<EffectsView>().Bind(_state, _entities);
            effects.AddComponent<GestureView>().Bind(_state, _entities);

            _follow = gameObject.AddComponent<FollowController>();
            _follow.Bind(_state, mapRoot.transform, mapAnchor);
            _locomotion = gameObject.AddComponent<PlayerLocomotion>();
            _locomotion.SetTableHeight(mapAnchor.y);
            _vignette = gameObject.AddComponent<ComfortVignette>();
            var hudRoot = new GameObject("Hud");
            hudRoot.transform.SetParent(transform, false);
            hudRoot.transform.SetPositionAndRotation(hudAnchor, userYaw);
            _hudRoot = hudRoot.transform;
            if (OVRManager.display != null)
                OVRManager.display.RecenteredPose += Reanchor;
            _hud = hudRoot.AddComponent<HudController>();
            _hud.Bind(_state);
            _voice = gameObject.AddComponent<BroadcastVoice>();
            _voice.Bind(_state);

            _hud.OnMenu = () => End(null);
            _hud.OnUntrack = Untrack;
            _hud.Build();


            laser.OnTileClicked = SelectTile;
            laser.OnEntityClicked = SelectEntity;
            // A miss click must not untrack while the trigger doubles as "eat".
            laser.OnMissClicked = () => { if (_manual == null) Untrack(); };
        }

        private void Update()
        {
            if (_ended || _bridge == null)
                return;

            int budget = MaxEventsPerFrame;
            while (budget-- > 0 && _bridge.TryReceive(out Dictionary<string, object> evt))
            {
                string type = Net.Json.Str(Net.Json.At(evt, "type"));
                if (_seenEventTypes.Add(type))
                    Debug.Log($"[Zappy] first '{type}' event received");
                _dispatcher.Dispatch(evt);
            }

            bool connected = _bridge.Connected;
            if (connected != _wasConnected)
            {
                _wasConnected = connected;
                _hud.SetConnected(connected);
            }
            if (!connected && _bridge.Error != null)
            {
                End(_bridge.Error);
                return;
            }
            if (_state.ServerClosed)
            {
                End("The bridge lost its zappy server");
                return;
            }

            _map.HoveredTile = _follow.Pov ? new Vector2Int(-1, -1) : _laser.Hovered;

            // X: leave the manual trantorian if controlling one, else release focus.
            if (OVRInput.GetDown(OVRInput.Button.One, OVRInput.Controller.LTouch)
                || (Keyboard.current != null && Keyboard.current.xKey.wasPressedThisFrame))
            {
                if (_manual != null)
                    DisconnectManual();
                else
                    Untrack();
            }

            // B always resets the camera view: exit POV, release focus, stand back
            // in front of the map (a manual connection, if any, stays alive).
            if (OVRInput.GetDown(OVRInput.Button.Two, OVRInput.Controller.RTouch)
                || (Keyboard.current != null && Keyboard.current.rKey.wasPressedThisFrame))
            {
                Untrack(); // Detach also leaves POV; the transition block restores HUD/laser.
                _locomotion.ResetView();
            }

            // A joins through the egg on the inspected tile (hint shown on the panel).
            if (_manual == null
                && (OVRInput.GetDown(OVRInput.Button.One, OVRInput.Controller.RTouch)
                    || (Keyboard.current != null && Keyboard.current.cKey.wasPressedThisFrame))
                && _hud.ConnectableEggShown)
                ConnectManual();

            // Y toggles the trantorian POV view: on the manually controlled player
            // (tracked only for the POV's duration, so the tabletop map stays put)
            // or on the focused player otherwise.
            if (OVRInput.GetDown(OVRInput.Button.Two, OVRInput.Controller.LTouch)
                || (Keyboard.current != null && Keyboard.current.yKey.wasPressedThisFrame))
            {
                if (_manualPlayerId >= 0)
                {
                    if (_follow.Pov)
                    {
                        _follow.Detach(); // Leaves POV and releases the map.
                    }
                    else
                    {
                        _follow.Track(_manualPlayerId);
                        _follow.TogglePov();
                    }
                }
                else if (_follow.TrackedPlayer >= 0)
                {
                    _follow.TogglePov();
                }
            }

            // While controlling a trantorian, the trigger (the tile-select press)
            // eats instead of selecting: Take food, gesture played off the pgt event.
            if (_manual != null
                && (OVRInput.GetDown(OVRInput.Button.PrimaryIndexTrigger, OVRInput.Controller.RTouch)
                    || (Keyboard.current != null && Keyboard.current.fKey.wasPressedThisFrame)))
                SendManual("Take food");

            // The joystick stays inactive while seeing through a trantorian, and it
            // belongs to the manual trantorian while one is controlled.
            _locomotion.enabled = !_follow.Pov && _manual == null;

            // POV embodiment: no HUD, no laser, no dimming, no own body — only the
            // head looks around. Everything comes back the moment POV ends.
            if (_follow.Pov != _povActive)
            {
                _povActive = _follow.Pov;
                _hudRoot.gameObject.SetActive(!_povActive);
                _laser.gameObject.SetActive(!_povActive);
                _entities.SetPovPlayer(_povActive ? _follow.TrackedPlayer : -1);
            }

            // Comfort vignette while the world moves under the user (POV or stick).
            _vignette.SetEngaged(_follow.Pov || _locomotion.IsMoving);

            // Broadcast voices are heard through the manual player's ears, or the
            // followed one's; at the free tabletop no one is listening.
            _voice.ListenerPlayer = _manualPlayerId >= 0 ? _manualPlayerId : _follow.TrackedPlayer;

            UpdateManual();
        }

        /// <summary>
        /// Join the game (the server picks which of the team's eggs hatches). The
        /// team is assigned automatically: fewest members, random among ties.
        /// </summary>
        private void ConnectManual()
        {
            if (_manual != null)
                return;
            string team = PickUnderdogTeam();
            if (string.IsNullOrEmpty(team))
                return;
            _manualTeam = team;
            _manualPlayerId = -1;
            _manualBusy = false;
            _idsBeforeJoin.Clear();
            foreach (int id in _state.Players.Keys)
                _idsBeforeJoin.Add(id);
            _manual = new PlayerClient();
            _manual.Connect(_host, GameServerPort, team);
            _hud.CloseInfo();
        }

        /// <summary>The team with the fewest live players; ties resolved randomly.</summary>
        private string PickUnderdogTeam()
        {
            var tied = new List<string>();
            int fewest = int.MaxValue;
            foreach (string team in _state.Teams)
            {
                int members = 0;
                foreach (PlayerData player in _state.Players.Values)
                {
                    if (player.Team == team)
                        members++;
                }
                if (members < fewest)
                {
                    fewest = members;
                    tied.Clear();
                }
                if (members == fewest)
                    tied.Add(team);
            }
            return tied.Count == 0 ? null : tied[UnityEngine.Random.Range(0, tied.Count)];
        }

        private void UpdateManual()
        {
            if (_manual == null)
                return;
            while (_manual.TryReceive(out string line))
            {
                // Any direct reply releases the command gate; the unsolicited lines
                // (incoming broadcasts, ejections, elevation results) do not.
                bool unsolicited = line.StartsWith("message") || line.StartsWith("eject:") || line.StartsWith("Current level");
                if (!unsolicited)
                    _manualBusy = false;
            }

            if (_manual.State == PlayerClient.Phase.Refused || _manual.State == PlayerClient.Phase.Lost)
            {
                Debug.Log($"[Zappy] manual player ended: {_manual.Error ?? "connection lost"}");
                DisconnectManual();
                return;
            }
            if (_manual.State == PlayerClient.Phase.Dead)
            {
                Debug.Log("[Zappy] manual trantorian died");
                DisconnectManual();
                return;
            }

            // The AI protocol never says which player id we are: adopt the first new
            // player of our team that appears in the GUI feed after joining.
            if (_manualPlayerId < 0 && _manual.State == PlayerClient.Phase.Joined)
            {
                foreach (KeyValuePair<int, PlayerData> pair in _state.Players)
                {
                    if (pair.Value.Team != _manualTeam || _idsBeforeJoin.Contains(pair.Key))
                        continue;
                    _manualPlayerId = pair.Key;
                    _entities.SetFocus(pair.Key);
                    // Deliberately NOT tracked: the map stays put while the manual
                    // trantorian walks across it (follow mode would slide the whole
                    // map under the stick). POV via Y tracks it on demand.
                    _hud.ShowPlayerInfo(pair.Key);
                    break;
                }
            }

            DriveManual();
        }

        /// <summary>
        /// The joystick drives the trantorian: stick up walks Forward, left/right
        /// turn. Commands wait for the previous reply, so holding the stick chains
        /// steps at the pace the server allows. Editor: W/A/D.
        /// </summary>
        private void DriveManual()
        {
            if (_manualBusy || _manual.State != PlayerClient.Phase.Joined)
                return;
            Vector2 stick = OVRInput.Get(OVRInput.Axis2D.PrimaryThumbstick, OVRInput.Controller.LTouch)
                            + OVRInput.Get(OVRInput.Axis2D.PrimaryThumbstick, OVRInput.Controller.RTouch);
            Keyboard keyboard = Keyboard.current;
            if (keyboard != null)
            {
                stick.x += (keyboard.dKey.isPressed ? 1f : 0f) - (keyboard.aKey.isPressed ? 1f : 0f);
                stick.y += keyboard.wKey.isPressed ? 1f : 0f;
            }
            if (stick.y > 0.6f)
                SendManual("Forward");
            else if (stick.x < -0.6f)
                SendManual("Left");
            else if (stick.x > 0.6f)
                SendManual("Right");
        }

        private void SendManual(string command)
        {
            if (_manual == null || _manual.State != PlayerClient.Phase.Joined || _manualBusy)
                return;
            _manualBusy = true;
            _manual.Send(command);
        }

        /// <summary>X while controlling, or any terminal client state.</summary>
        private void DisconnectManual()
        {
            _manual?.Dispose();
            _manual = null;
            _manualTeam = null;
            _manualPlayerId = -1;
            _manualBusy = false;
            Untrack();
        }

        private void SelectTile(int x, int y)
        {
            if (_manual != null)
                return; // The trigger belongs to "eat" during manual control.
            Untrack();
            _map.SelectedTile = new Vector2Int(x, y);
            _hud.ShowTileInfo(x, y);
        }

        /// <summary>Browser behavior: clicking a player focuses, dims the rest and follows it.</summary>
        private void SelectEntity(EntityMarker marker)
        {
            if (_manual != null || marker.Type != "player")
                return; // The trigger belongs to "eat" during manual control.
            _map.SelectedTile = new Vector2Int(-1, -1);
            _entities.SetFocus(marker.Number);
            _follow.Track(marker.Number);
            _hud.ShowPlayerInfo(marker.Number);
        }

        /// <summary>Map/HUD anchors and yaw derived from the current head pose.</summary>
        private static (Vector3 map, Vector3 hud, Quaternion yaw) UserFrame()
        {
            Camera head = Camera.main;
            if (head == null)
                return (MapAnchor, HudAnchor, Quaternion.identity);
            Vector3 forward = head.transform.forward;
            forward.y = 0f;
            if (forward.sqrMagnitude < 0.001f)
                return (MapAnchor, HudAnchor, Quaternion.identity);
            var yaw = Quaternion.LookRotation(forward.normalized);
            Vector3 feet = head.transform.position;
            feet.y = 0f;
            return (feet + yaw * MapAnchor, feet + yaw * HudAnchor, yaw);
        }

        /// <summary>Recenter moves the world under the user; put the table back in front.</summary>
        private void Reanchor()
        {
            (Vector3 mapAnchor, Vector3 hudAnchor, Quaternion userYaw) = UserFrame();
            _follow.SetAnchor(mapAnchor);
            if (_hudRoot != null)
                _hudRoot.SetPositionAndRotation(hudAnchor, userYaw);
        }

        private void Untrack()
        {
            _map.SelectedTile = new Vector2Int(-1, -1);
            _entities.ClearFocus();
            _follow.Detach();
            _hud.CloseInfo();
        }

        private void End(string error)
        {
            if (_ended)
                return;
            _ended = true;
            // The menu needs the laser back if the session dies mid-POV.
            _laser.gameObject.SetActive(true);
            _laser.OnTileClicked = null;
            _laser.OnEntityClicked = null;
            _laser.OnMissClicked = null;
            _manual?.Dispose();
            _manual = null;
            _bridge.Dispose();
            _bridge = null;
            OnEnded?.Invoke(error);
        }

        private void OnDestroy()
        {
            if (OVRManager.display != null)
                OVRManager.display.RecenteredPose -= Reanchor;
            _manual?.Dispose();
            _bridge?.Dispose();
        }
    }
}
