using System.Collections.Generic;
using UnityEngine;
using Zappy.Model;
using Zappy.Net;

namespace Zappy.Protocol
{
    /// <summary>
    /// Applies the bridge's JSON events to the GameState — the exact event set
    /// bridge.js emits (WELCOME snapshot, then per-message GRAPHIC translations).
    /// </summary>
    public class EventDispatcher
    {
        private readonly GameState _state;

        public EventDispatcher(GameState state)
        {
            _state = state;
        }

        public void Dispatch(Dictionary<string, object> evt)
        {
            var data = Json.Dict(Json.At(evt, "data"));
            switch (Json.Str(Json.At(evt, "type")))
            {
                case "WELCOME": ApplySnapshot(data); break;
                case "msz": _state.Resize(Json.Int(Json.At(data, "x")), Json.Int(Json.At(data, "y"))); break;
                case "bct": ApplyTile(data); break;
                case "tna": AddTeam(Json.Str(Json.At(data, "name"))); break;
                case "pnw": ApplyPlayer(data); break;
                case "ppo": MovePlayer(data, "Moved"); break;
                case "pipi": ApplyPlayer(data); break;
                case "plv": SetLevel(data); break;
                case "pdr": SetResourceAction(data, "Dropped resource"); break;
                case "pgt": SetResourceAction(data, "Collected resource"); break;
                case "pex": SetAction(data, "Ejected"); break;
                case "pfk": SetAction(data, "Laid an egg"); break;
                case "pbc": _state.AddBroadcast(Json.Int(Json.At(data, "id")), Decode(Json.Str(Json.At(data, "message")))); break;
                case "pic": SetIncanting(data, true); break;
                case "pie": SetIncanting(data, false); break;
                case "pdi": RemovePlayer(Json.Int(Json.At(data, "id"))); break;
                case "enw": ApplyEgg(data); break;
                case "ebo": RemoveEgg(Json.Int(Json.At(data, "eggId"))); break;
                case "edi": RemoveEgg(Json.Int(Json.At(data, "eggId"))); break;
                case "sgt": _state.TimeUnit = Json.Int(Json.At(data, "timeUnit")); _state.Touch(); break;
                case "sst": _state.TimeUnit = Json.Int(Json.At(data, "timeUnit")); _state.Touch(); break;
                case "seg": _state.Winner = Json.Str(Json.At(data, "teamName")); _state.Touch(); break;
                case "players_sync": ApplyPlayersSync(data); break;
                case "server_closed": _state.ServerClosed = true; _state.Touch(); break;
            }
        }

        /// <summary>The bridge's AI messages use underscores for spaces.</summary>
        private static string Decode(string message) => message.Replace('_', ' ');

        private void ApplySnapshot(Dictionary<string, object> data)
        {
            if (data == null)
                return;
            var mapSize = Json.Dict(Json.At(data, "mapSize"));
            _state.Resize(Json.Int(Json.At(mapSize, "x")), Json.Int(Json.At(mapSize, "y")));

            var teams = Json.List(Json.At(data, "teams"));
            if (teams != null)
                foreach (object team in teams)
                    AddTeam(Json.Str(team));

            var tiles = Json.Dict(Json.At(data, "tiles"));
            if (tiles != null)
                foreach (KeyValuePair<string, object> pair in tiles)
                    ApplyTile(Json.Dict(pair.Value));

            var players = Json.Dict(Json.At(data, "players"));
            if (players != null)
                foreach (KeyValuePair<string, object> pair in players)
                    ApplyPlayer(Json.Dict(pair.Value));

            var eggs = Json.Dict(Json.At(data, "eggs"));
            if (eggs != null)
                foreach (KeyValuePair<string, object> pair in eggs)
                    ApplyEgg(Json.Dict(pair.Value));

            _state.TimeUnit = Json.Int(Json.At(data, "timeUnit"));
            _state.Touch();
            Debug.Log($"[Zappy] snapshot: map {_state.Width}x{_state.Height}, {_state.Players.Count} players, {_state.Eggs.Count} eggs, {_state.Teams.Count} teams");
        }

        private void ApplyTile(Dictionary<string, object> data)
        {
            if (data == null)
                return;
            int x = Json.Int(Json.At(data, "x"));
            int y = Json.Int(Json.At(data, "y"));
            if (!_state.InBounds(x, y))
                return;
            var resources = Json.Dict(Json.At(data, "resources"));
            TileData tile = _state.TileAt(x, y);
            for (int i = 0; i < ResourceInfo.Count; i++)
                tile.Resources[i] = Json.Int(Json.At(resources, ResourceInfo.Names[i]));
            _state.Touch();
        }

        private void AddTeam(string name)
        {
            if (string.IsNullOrEmpty(name) || _state.Teams.Contains(name))
                return;
            _state.Teams.Add(name);
            _state.Touch();
        }

        /// <summary>pnw and pipi carry the full pose; pipi has no team, keep the known one.</summary>
        private void ApplyPlayer(Dictionary<string, object> data)
        {
            if (data == null)
                return;
            int id = Json.Int(Json.At(data, "id"));
            if (!_state.Players.TryGetValue(id, out PlayerData player))
            {
                player = new PlayerData { Id = id };
                _state.Players[id] = player;
            }
            player.X = Json.Int(Json.At(data, "x"));
            player.Y = Json.Int(Json.At(data, "y"));
            player.Orientation = Json.Int(Json.At(data, "orientation"));
            player.Level = Json.Int(Json.At(data, "level"));
            string team = Json.Str(Json.At(data, "team"));
            if (team.Length > 0)
                player.Team = team;
            _state.Touch();
        }

        private void MovePlayer(Dictionary<string, object> data, string action)
        {
            if (data == null || !_state.Players.TryGetValue(Json.Int(Json.At(data, "id")), out PlayerData player))
                return;
            player.X = Json.Int(Json.At(data, "x"));
            player.Y = Json.Int(Json.At(data, "y"));
            player.Orientation = Json.Int(Json.At(data, "orientation"));
            player.LastAction = action;
            _state.Touch();
        }

        private void SetLevel(Dictionary<string, object> data)
        {
            if (data == null || !_state.Players.TryGetValue(Json.Int(Json.At(data, "id")), out PlayerData player))
                return;
            player.Level = Json.Int(Json.At(data, "level"));
            player.LastAction = "Level Up";
            _state.Touch();
        }

        private void SetAction(Dictionary<string, object> data, string action)
        {
            if (data == null || !_state.Players.TryGetValue(Json.Int(Json.At(data, "id")), out PlayerData player))
                return;
            player.LastAction = action;
            player.ActionStamp++;
            _state.Touch();
        }

        /// <summary>pgt/pdr also say which resource; gestures need it (food vs mineral).</summary>
        private void SetResourceAction(Dictionary<string, object> data, string action)
        {
            if (data == null || !_state.Players.TryGetValue(Json.Int(Json.At(data, "id")), out PlayerData player))
                return;
            player.LastAction = action;
            player.LastResource = Json.Int(Json.At(data, "resource"));
            player.ActionStamp++;
            _state.Touch();
        }

        private void SetIncanting(Dictionary<string, object> data, bool incanting)
        {
            if (data == null)
                return;
            int x = Json.Int(Json.At(data, "x"));
            int y = Json.Int(Json.At(data, "y"));
            if (!_state.InBounds(x, y))
                return;
            _state.TileAt(x, y).Incanting = incanting;
            _state.Touch();
        }

        private void RemovePlayer(int id)
        {
            if (_state.Players.Remove(id))
                _state.Touch();
        }

        private void ApplyEgg(Dictionary<string, object> data)
        {
            if (data == null)
                return;
            // The live enw event says "eggId"; the WELCOME snapshot stores eggs as "id".
            object rawId = Json.At(data, "eggId") ?? Json.At(data, "id");
            int id = Json.Int(rawId);
            _state.Eggs[id] = new EggData
            {
                Id = id,
                PlayerId = Json.Int(Json.At(data, "playerId")),
                X = Json.Int(Json.At(data, "x")),
                Y = Json.Int(Json.At(data, "y"))
            };
            _state.Touch();
        }

        private void RemoveEgg(int id)
        {
            if (_state.Eggs.Remove(id))
                _state.Touch();
        }

        private void ApplyPlayersSync(Dictionary<string, object> data)
        {
            if (data == null)
                return;
            foreach (KeyValuePair<string, object> pair in data)
                ApplyPlayer(Json.Dict(pair.Value));
        }
    }
}
