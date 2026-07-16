using System;
using System.Collections.Generic;
using UnityEngine;
using Zappy.Model;
using Zappy.View;

namespace Zappy.VRUI
{
    /// <summary>
    /// VR counterpart of the browser viewer's overlays: status + team stats
    /// (top-left), broadcast log (top-right), tile/player info panel (bottom-left),
    /// the end-game banner (center) and a MENU button. Everything is world-space
    /// TextMesh rows refreshed from GameState.
    /// </summary>
    public class HudController : MonoBehaviour
    {
        private const int TeamLines = 6;
        private const int LogLines = 6;
        private const int PlayerLines = 4;

        public Action OnMenu;

        /// <summary>Clicking a player focuses and follows it (browser behavior).</summary>
        public Action OnUntrack;



        private GameState _state;
        private long _version = -1;

        private TextMesh _status;
        private TextMesh _playerCount;
        private readonly TextMesh[] _teamStats = new TextMesh[TeamLines];
        private readonly TextMesh[] _log = new TextMesh[LogLines];
        private long _lastLoggedSequence = -1;
        private readonly List<string> _logHistory = new();
        private readonly List<Color> _logColors = new();

        private GameObject _tilePanel;
        private TextMesh _tileTitle;
        private readonly TextMesh[] _tileResources = new TextMesh[ResourceInfo.Count];
        private readonly TextMesh[] _tilePlayers = new TextMesh[PlayerLines];
        private TextMesh _connectHint;
        private int _tileX = -1;
        private int _tileY = -1;

        private GameObject _playerPanel;
        private TextMesh _playerTitle;
        private TextMesh _playerTeam;
        private TextMesh _playerLevel;
        private TextMesh _playerAction;
        private int _trackedPlayer = -1;

        private Transform _infoAnchor;
        private GameObject _overlayPanel;
        private GameObject _banner;
        private TextMesh _bannerText;
        private bool _bannerDismissed;

        public void Bind(GameState state)
        {
            _state = state;
            _version = -1;
        }

        public void Build()
        {
            // Info cards take the system panel's slot: showing one hides the game
            // resume, closing it brings the resume back.
            var infoAnchor = new GameObject("InfoAnchor");
            infoAnchor.transform.SetParent(transform, false);
            // Exactly the system panel's position, so the swap is seamless.
            infoAnchor.transform.localPosition = new Vector3(-0.95f, 0.58f, -0.02f);
            _infoAnchor = infoAnchor.transform;

            BuildOverlay();
            BuildLog();
            BuildTilePanel();
            BuildPlayerPanel();
            BuildBanner();

            // In the gap between the two scoreboard panels, above the map.
            UIFactory.Button(transform, "MENU", new Vector3(0f, 0.67f, 0f), 0.5f, 0.11f, () => OnMenu?.Invoke());
        }

        private void BuildOverlay()
        {
            _overlayPanel = UIFactory.Panel(transform, "Overlay", 1.15f, 0.78f, UIFactory.PanelColor);
            GameObject panel = _overlayPanel;
            // High above the tabletop so a full team list never dips behind the map.
            panel.transform.localPosition = new Vector3(-0.95f, 0.58f, 0f);
            UIFactory.Label(panel.transform, "SYSTEM: ZAPPY BRIDGE", new Vector3(-0.52f, 0.30f, 0f), 0.045f, UIFactory.AccentColor, TextAnchor.MiddleLeft);
            _status = UIFactory.Label(panel.transform, "Status: DISCONNECTED", new Vector3(-0.52f, 0.21f, 0f), 0.042f, new Color(1f, 0.27f, 0.27f), TextAnchor.MiddleLeft);
            _playerCount = UIFactory.Label(panel.transform, "Players: 0", new Vector3(-0.52f, 0.12f, 0f), 0.042f, UIFactory.LabelColor, TextAnchor.MiddleLeft);
            UIFactory.Label(panel.transform, "ACTIVE TEAMS", new Vector3(-0.52f, 0.02f, 0f), 0.038f, new Color(1f, 0.84f, 0f), TextAnchor.MiddleLeft);
            for (int i = 0; i < TeamLines; i++)
                _teamStats[i] = UIFactory.Label(panel.transform, "", new Vector3(-0.52f, -0.06f - i * 0.055f, 0f), 0.036f, UIFactory.LabelColor, TextAnchor.MiddleLeft);
        }

        private void BuildLog()
        {
            GameObject panel = UIFactory.Panel(transform, "BroadcastLog", 1.6f, 0.78f, UIFactory.PanelColor);
            // Its left edge must clear the central button column (panel is 1.6 wide),
            // raised like the overlay so the log never dips behind the map.
            panel.transform.localPosition = new Vector3(1.2f, 0.58f, 0f);
            UIFactory.Label(panel.transform, "BROADCASTS", new Vector3(-0.74f, 0.30f, 0f), 0.045f, UIFactory.AccentColor, TextAnchor.MiddleLeft);
            for (int i = 0; i < LogLines; i++)
                _log[i] = UIFactory.Label(panel.transform, "", new Vector3(-0.74f, 0.20f - i * 0.093f, 0f), 0.036f, UIFactory.LabelColor, TextAnchor.MiddleLeft);
        }

        private void BuildTilePanel()
        {
            _tilePanel = UIFactory.Panel(_infoAnchor, "TileInfo", 1.15f, 1.15f, UIFactory.PanelColor);
            _tilePanel.transform.localPosition = new Vector3(0f, -0.18f, 0f);
            _tileTitle = UIFactory.Label(_tilePanel.transform, "TILE", new Vector3(-0.52f, 0.50f, 0f), 0.05f, UIFactory.AccentColor, TextAnchor.MiddleLeft);
            UIFactory.Button(_tilePanel.transform, "X", new Vector3(0.49f, 0.50f, 0f), 0.09f, 0.09f, CloseInfo);
            for (int i = 0; i < ResourceInfo.Count; i++)
            {
                float y = 0.38f - i * 0.085f;
                UIFactory.Icon(_tilePanel.transform, new Vector3(-0.48f, y, 0f), 0.055f, ResourceInfo.Colors[i]);
                _tileResources[i] = UIFactory.Label(_tilePanel.transform, ResourceInfo.Names[i], new Vector3(-0.41f, y, 0f), 0.038f, UIFactory.LabelColor, TextAnchor.MiddleLeft);
            }
            for (int i = 0; i < PlayerLines; i++)
                _tilePlayers[i] = UIFactory.Label(_tilePanel.transform, "", new Vector3(-0.48f, -0.28f - i * 0.06f, 0f), 0.034f, UIFactory.LabelColor, TextAnchor.MiddleLeft);
            // Shown when this tile holds an egg: press A on the controller to join
            // (the session assigns the team automatically).
            _connectHint = UIFactory.Label(_tilePanel.transform, "press (A) to connect", new Vector3(-0.48f, -0.51f, 0f), 0.038f, UIFactory.AccentColor, TextAnchor.MiddleLeft);
            _tilePanel.SetActive(false);
        }

        private void BuildPlayerPanel()
        {
            _playerPanel = UIFactory.Panel(_infoAnchor, "PlayerInfo", 1.15f, 0.72f, UIFactory.PanelColor);
            _playerPanel.transform.localPosition = Vector3.zero;
            _playerTitle = UIFactory.Label(_playerPanel.transform, "PLAYER", new Vector3(-0.52f, 0.27f, 0f), 0.05f, new Color(1f, 0.84f, 0f), TextAnchor.MiddleLeft);
            UIFactory.Button(_playerPanel.transform, "X", new Vector3(0.49f, 0.27f, 0f), 0.09f, 0.09f, () => OnUntrack?.Invoke());
            _playerTeam = UIFactory.Label(_playerPanel.transform, "", new Vector3(-0.52f, 0.13f, 0f), 0.042f, UIFactory.LabelColor, TextAnchor.MiddleLeft);
            _playerLevel = UIFactory.Label(_playerPanel.transform, "", new Vector3(-0.52f, 0.03f, 0f), 0.042f, UIFactory.LabelColor, TextAnchor.MiddleLeft);
            _playerAction = UIFactory.Label(_playerPanel.transform, "", new Vector3(-0.52f, -0.07f, 0f), 0.042f, UIFactory.LabelColor, TextAnchor.MiddleLeft);
            UIFactory.Label(_playerPanel.transform, "(following - X release, Y POV)", new Vector3(-0.52f, -0.21f, 0f), 0.032f, new Color(0.6f, 0.6f, 0.65f), TextAnchor.MiddleLeft);
            _playerPanel.SetActive(false);
        }

        private void BuildBanner()
        {
            _banner = UIFactory.Panel(transform, "EndGameBanner", 2.3f, 0.8f, new Color(0.02f, 0.02f, 0.03f, 0.97f));
            _banner.transform.localPosition = new Vector3(0f, 0.3f, -0.1f);
            UIFactory.Label(_banner.transform, "SIMULATION ENDED", new Vector3(0f, 0.22f, 0f), 0.08f, new Color(1f, 0.84f, 0f));
            _bannerText = UIFactory.Label(_banner.transform, "", new Vector3(0f, 0.02f, 0f), 0.065f, UIFactory.LabelColor);
            UIFactory.Button(_banner.transform, "CLOSE", new Vector3(0f, -0.26f, 0f), 0.5f, 0.1f, () =>
            {
                _bannerDismissed = true;
                _banner.SetActive(false);
            });
            _banner.SetActive(false);
        }

        public void SetConnected(bool connected)
        {
            _status.text = connected ? "Status: CONNECTED" : "Status: DISCONNECTED";
            _status.color = connected ? new Color(0.27f, 1f, 0.27f) : new Color(1f, 0.27f, 0.27f);
        }

        public void ShowTileInfo(int x, int y)
        {
            _trackedPlayer = -1;
            _playerPanel.SetActive(false);
            _overlayPanel.SetActive(false); // The card takes the system panel's place.
            _tileX = x;
            _tileY = y;
            _tilePanel.SetActive(true);
            RefreshTileInfo();
        }

        public void ShowPlayerInfo(int id)
        {
            _tilePanel.SetActive(false);
            _overlayPanel.SetActive(false); // The card takes the system panel's place.
            _trackedPlayer = id;
            _playerPanel.SetActive(true);
            RefreshPlayerInfo();
        }

        /// <summary>True when the shown tile offers an egg to join through (press A).</summary>
        public bool ConnectableEggShown => _tilePanel.activeInHierarchy && _connectHint.gameObject.activeSelf;

        public void CloseInfo()
        {
            _tilePanel.SetActive(false);
            _playerPanel.SetActive(false);
            _overlayPanel.SetActive(true);
            _trackedPlayer = -1;
            _tileX = -1;
        }

        private void Update()
        {
            if (_state == null || _state.Version == _version)
                return;
            _version = _state.Version;
            RefreshOverlay();
            RefreshLog();
            if (_tilePanel.activeSelf)
                RefreshTileInfo();
            if (_playerPanel.activeSelf)
                RefreshPlayerInfo();
            RefreshBanner();
        }

        private void RefreshOverlay()
        {
            _playerCount.text = $"Players: {_state.Players.Count}";

            var counts = new Dictionary<string, int>();
            foreach (PlayerData player in _state.Players.Values)
                counts[player.Team] = counts.GetValueOrDefault(player.Team) + 1;
            int line = 0;
            foreach (KeyValuePair<string, int> pair in counts)
            {
                if (line >= TeamLines)
                    break;
                _teamStats[line].text = $"{pair.Key}: {pair.Value}";
                _teamStats[line].color = TeamColors.Get(pair.Key);
                line++;
            }
            for (; line < TeamLines; line++)
                _teamStats[line].text = "";
        }

        private void RefreshLog()
        {
            foreach (BroadcastEntry broadcast in _state.Broadcasts)
            {
                if (broadcast.Sequence <= _lastLoggedSequence)
                    continue;
                _lastLoggedSequence = broadcast.Sequence;
                string text = broadcast.Message.Length > 42 ? broadcast.Message.Substring(0, 42) + "…" : broadcast.Message;
                _logHistory.Insert(0, $"P{broadcast.PlayerId} [{broadcast.Team}] {text}");
                _logColors.Insert(0, TeamColors.Get(broadcast.Team));
                if (_logHistory.Count > LogLines)
                {
                    _logHistory.RemoveAt(LogLines);
                    _logColors.RemoveAt(LogLines);
                }
            }
            for (int i = 0; i < LogLines; i++)
            {
                _log[i].text = i < _logHistory.Count ? _logHistory[i] : "";
                if (i < _logColors.Count)
                    _log[i].color = _logColors[i];
            }
        }

        private void RefreshTileInfo()
        {
            if (!_state.InBounds(_tileX, _tileY))
                return;
            _tileTitle.text = $"TILE ({_tileX}, {_tileY})";
            TileData tile = _state.TileAt(_tileX, _tileY);
            for (int i = 0; i < ResourceInfo.Count; i++)
                _tileResources[i].text = $"{ResourceInfo.Names[i]}: {tile.Resources[i]}";

            bool hasEgg = false;
            foreach (EggData egg in _state.Eggs.Values)
            {
                if (egg.X == _tileX && egg.Y == _tileY)
                {
                    hasEgg = true;
                    break;
                }
            }
            _connectHint.gameObject.SetActive(hasEgg);

            var here = new List<PlayerData>();
            foreach (PlayerData player in _state.Players.Values)
            {
                if (player.X == _tileX && player.Y == _tileY)
                    here.Add(player);
            }
            for (int i = 0; i < PlayerLines; i++)
            {
                if (i < here.Count)
                {
                    _tilePlayers[i].text = $"P{here[i].Id}  {here[i].Team}  Lv.{here[i].Level}";
                    _tilePlayers[i].color = TeamColors.Get(here[i].Team);
                }
                else
                {
                    _tilePlayers[i].text = i == 0 ? "no players here" : "";
                    _tilePlayers[i].color = new Color(0.6f, 0.6f, 0.65f);
                }
            }
        }

        private void RefreshPlayerInfo()
        {
            if (!_state.Players.TryGetValue(_trackedPlayer, out PlayerData player))
            {
                CloseInfo(); // Died while tracked.
                OnUntrack?.Invoke();
                return;
            }
            _playerTitle.text = $"PLAYER {player.Id}";
            _playerTeam.text = $"Team: {player.Team}";
            _playerTeam.color = TeamColors.Get(player.Team);
            _playerLevel.text = $"Level: {player.Level}";
            _playerAction.text = $"Last action: {player.LastAction}";
        }

        private void RefreshBanner()
        {
            if (string.IsNullOrEmpty(_state.Winner) || _bannerDismissed || _banner.activeSelf)
                return;
            _bannerText.text = $"Team {_state.Winner} has won!";
            _bannerText.color = TeamColors.Get(_state.Winner);
            _banner.SetActive(true);
        }
    }
}
