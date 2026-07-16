using System.Collections.Generic;
using UnityEngine;

namespace Zappy.Model
{
    public class PlayerData
    {
        public int Id;
        public int X;
        public int Y;
        public int Orientation; // 1 N, 2 E, 3 S, 4 W (grid north = -Z in map space)
        public int Level;
        public string Team = "";
        public string LastAction = "Spawned";
        public long ActionStamp;    // Bumped on every action so repeats still trigger.
        public int LastResource = -1; // Resource index of the last pgt/pdr.
    }

    public class EggData
    {
        public int Id;
        public int PlayerId;
        public int X;
        public int Y;
    }

    public class TileData
    {
        public readonly int[] Resources = new int[ResourceInfo.Count];
        public bool Incanting;
    }

    public class BroadcastEntry
    {
        public long Sequence;
        public int PlayerId;
        public string Team = "";
        public string Message = "";
    }

    /// <summary>Names and legend colors of the seven resources (browser GUI legend).</summary>
    public static class ResourceInfo
    {
        public const int Count = 7;
        public const int FoodIndex = 0;

        public static readonly string[] Names =
        {
            "food", "linemate", "deraumere", "sibur", "mendiane", "phiras", "thystame"
        };

        public static readonly Color[] Colors =
        {
            new Color(0.53f, 0.80f, 0.27f), // food      #88cc44
            new Color(1.00f, 0.20f, 0.20f), // linemate  #ff3333
            new Color(0.53f, 0.53f, 0.53f), // deraumere #888888
            new Color(1.00f, 1.00f, 0.00f), // sibur     #ffff00
            new Color(0.00f, 1.00f, 0.00f), // mendiane  #00ff00
            new Color(0.93f, 0.51f, 0.93f), // phiras    #ee82ee
            new Color(1.00f, 1.00f, 1.00f)  // thystame  #ffffff
        };
    }

    /// <summary>Port of the browser GUI's getTeamColor (same hash, same 16-color palette).</summary>
    public static class TeamColors
    {
        private static readonly Color[] Palette =
        {
            Hex(0xFF3333), Hex(0x33FF33), Hex(0x3333FF), Hex(0xFFFF33),
            Hex(0xFF33FF), Hex(0x33FFFF), Hex(0xFFAA33), Hex(0xAA33AA),
            Hex(0x33AA33), Hex(0x3333AA), Hex(0xAA3333), Hex(0xAAAA33),
            Hex(0x33AAAA), Hex(0xFFB6C1), Hex(0x7B68EE), Hex(0xFA8072)
        };

        public static Color Get(string team)
        {
            if (string.IsNullOrEmpty(team))
                return Color.white;
            int hash = 0;
            unchecked
            {
                foreach (char c in team)
                    hash = c + ((hash << 5) - hash);
                if (hash == int.MinValue)
                    hash = 0;
            }
            return Palette[Mathf.Abs(hash) % Palette.Length];
        }

        private static Color Hex(int rgb)
        {
            return new Color(((rgb >> 16) & 0xff) / 255f, ((rgb >> 8) & 0xff) / 255f, (rgb & 0xff) / 255f);
        }
    }

    /// <summary>Ball size and color per level (trantorians are level-styled spheres).</summary>
    public static class LevelStyle
    {
        private static readonly Color[] Colors =
        {
            new Color(0.75f, 0.75f, 0.75f), // 1 silver
            new Color(0.30f, 0.69f, 0.31f), // 2 green
            new Color(0.00f, 0.74f, 0.83f), // 3 cyan
            new Color(0.13f, 0.59f, 0.95f), // 4 blue
            new Color(0.61f, 0.15f, 0.69f), // 5 purple
            new Color(0.91f, 0.12f, 0.39f), // 6 pink
            new Color(1.00f, 0.60f, 0.00f), // 7 orange
            new Color(1.00f, 0.84f, 0.00f)  // 8 gold
        };

        public static Color ColorOf(int level) => Colors[Mathf.Clamp(level, 1, Colors.Length) - 1];

        /// <summary>
        /// Ball diameter in map units (one unit per tile). Starts at half a tile so
        /// even level 1 clearly dwarfs the resource props (max 0.22 units).
        /// </summary>
        public static float Diameter(int level) => 0.42f + 0.06f * Mathf.Clamp(level, 1, 8);
    }

    /// <summary>
    /// The mirrored bridge state. Mutated by EventDispatcher on the main thread;
    /// views resynchronize whenever Version changes.
    /// </summary>
    public class GameState
    {
        private const int MaxBroadcasts = 32;

        public int Width { get; private set; }
        public int Height { get; private set; }
        public TileData[,] Tiles { get; private set; } = new TileData[0, 0];
        public readonly Dictionary<int, PlayerData> Players = new();
        public readonly Dictionary<int, EggData> Eggs = new();
        public readonly List<string> Teams = new();
        public readonly List<BroadcastEntry> Broadcasts = new();
        public int TimeUnit = 100;
        public string Winner;      // Set by seg: the simulation ended.
        public bool ServerClosed;  // The bridge lost its zappy_server.
        public long Version { get; private set; }

        private long _broadcastSequence;

        public void Touch() => Version++;

        public bool InBounds(int x, int y) => x >= 0 && x < Width && y >= 0 && y < Height;

        public TileData TileAt(int x, int y) => Tiles[x, y];

        public void Resize(int width, int height)
        {
            if (width == Width && height == Height)
                return;
            Width = width;
            Height = height;
            Tiles = new TileData[width, height];
            for (int x = 0; x < width; x++)
                for (int y = 0; y < height; y++)
                    Tiles[x, y] = new TileData();
            Touch();
        }

        public void AddBroadcast(int playerId, string message)
        {
            string team = Players.TryGetValue(playerId, out PlayerData player) ? player.Team : "";
            Broadcasts.Add(new BroadcastEntry
            {
                Sequence = _broadcastSequence++,
                PlayerId = playerId,
                Team = team,
                Message = message
            });
            if (Broadcasts.Count > MaxBroadcasts)
                Broadcasts.RemoveAt(0);
            Touch();
        }
    }
}
