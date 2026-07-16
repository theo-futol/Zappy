using System.Collections.Generic;
using UnityEngine;
using Zappy.Model;

namespace Zappy.View
{
    /// <summary>
    /// The browser viewer's world, in 3D: a grass checker with a water border, and
    /// the tile resources as colored primitives — food is a cylinder, every mineral
    /// a colored cube ("square"), sized up a little with the stack count.
    /// One world unit per tile; the map root is scaled to tabletop size by the session.
    /// </summary>
    public class MapView : MonoBehaviour
    {
        private const int WaterPadding = 2;

        private static readonly Color GrassA = new Color(0.30f, 0.42f, 0.25f);
        private static readonly Color GrassB = new Color(0.36f, 0.50f, 0.30f);
        private static readonly Color WaterColor = new Color(0.16f, 0.32f, 0.55f);
        private static readonly Color HoverColor = new Color(0.55f, 0.65f, 0.45f);
        private static readonly Color SelectedColor = new Color(0.95f, 0.65f, 0.15f);
        private static readonly Color IncantColor = new Color(1.00f, 0.00f, 1.00f); // Browser's 0xff00ff tint

        // The 7 resource slots per tile, browser layout: 3 / 2 / 2.
        private static readonly Vector2[] Slots =
        {
            new Vector2(-0.28f, -0.28f), new Vector2(0f, -0.28f), new Vector2(0.28f, -0.28f),
            new Vector2(-0.28f, 0f), new Vector2(0.28f, 0f),
            new Vector2(-0.28f, 0.28f), new Vector2(0.28f, 0.28f)
        };

        private GameState _state;
        private Renderer[,] _tileRenderers;
        private GameObject[,] _propRoots;
        private int[][,] _shownResources;
        private int _width;
        private int _height;
        private long _version = -1;

        public Vector2Int SelectedTile = new Vector2Int(-1, -1);
        public Vector2Int HoveredTile = new Vector2Int(-1, -1);

        public void Bind(GameState state)
        {
            _state = state;
            _version = -1;
        }

        /// <summary>Center of a tile in map-root local space (y up, one unit per tile).</summary>
        public static Vector3 TileCenter(int x, int y)
        {
            return new Vector3(x, 0f, y);
        }

        private void Update()
        {
            if (_state == null)
                return;
            if (_state.Width != _width || _state.Height != _height)
                Rebuild();
            if (_state.Version != _version)
            {
                _version = _state.Version;
                RefreshResources();
            }
            RefreshTints();
        }

        private Transform _tilesRoot;

        private void Rebuild()
        {
            // Only rebuild OUR container: EntitiesView/EffectsView live as sibling
            // children of the map root and must survive a map resize.
            if (_tilesRoot != null)
                Destroy(_tilesRoot.gameObject);
            _tilesRoot = new GameObject("Tiles").transform;
            _tilesRoot.SetParent(transform, false);
            _width = _state.Width;
            _height = _state.Height;
            _tileRenderers = new Renderer[_width, _height];
            _propRoots = new GameObject[_width, _height];
            _shownResources = new int[ResourceInfo.Count][,];
            for (int i = 0; i < ResourceInfo.Count; i++)
                _shownResources[i] = new int[_width, _height];
            if (_width == 0 || _height == 0)
                return;

            for (int x = -WaterPadding; x < _width + WaterPadding; x++)
            {
                for (int y = -WaterPadding; y < _height + WaterPadding; y++)
                {
                    bool water = x < 0 || x >= _width || y < 0 || y >= _height;
                    GameObject tile = GameObject.CreatePrimitive(PrimitiveType.Cube);
                    tile.name = water ? "Water" : $"Tile_{x}_{y}";
                    tile.transform.SetParent(_tilesRoot, false);
                    tile.transform.localPosition = TileCenter(x, y) + (water ? new Vector3(0f, -0.02f, 0f) : Vector3.zero);
                    tile.transform.localScale = new Vector3(water ? 1f : 0.96f, 0.08f, water ? 1f : 0.96f);

                    if (water)
                    {
                        Destroy(tile.GetComponent<Collider>()); // Water is scenery, not clickable.
                        tile.GetComponent<Renderer>().sharedMaterial = MaterialCache.GetLit(WaterColor);
                        continue;
                    }

                    tile.GetComponent<Renderer>().sharedMaterial = MaterialCache.GetLit((x + y) % 2 == 0 ? GrassA : GrassB);
                    var marker = tile.AddComponent<TileMarker>();
                    marker.X = x;
                    marker.Y = y;
                    _tileRenderers[x, y] = tile.GetComponent<Renderer>();

                    var props = new GameObject($"Props_{x}_{y}");
                    props.transform.SetParent(_tilesRoot, false);
                    props.transform.localPosition = TileCenter(x, y);
                    _propRoots[x, y] = props;
                    // Force the first RefreshResources to populate every slot.
                    for (int i = 0; i < ResourceInfo.Count; i++)
                        _shownResources[i][x, y] = -1;
                }
            }
        }

        private void RefreshResources()
        {
            if (_width == 0 || _height == 0)
                return;
            for (int x = 0; x < _width; x++)
            {
                for (int y = 0; y < _height; y++)
                {
                    TileData tile = _state.TileAt(x, y);
                    bool changed = false;
                    for (int i = 0; i < ResourceInfo.Count; i++)
                        changed |= _shownResources[i][x, y] != tile.Resources[i];
                    if (!changed)
                        continue;

                    GameObject root = _propRoots[x, y];
                    foreach (Transform child in root.transform)
                        Destroy(child.gameObject);
                    for (int i = 0; i < ResourceInfo.Count; i++)
                    {
                        _shownResources[i][x, y] = tile.Resources[i];
                        if (tile.Resources[i] > 0)
                            SpawnProp(root.transform, i, tile.Resources[i]);
                    }
                }
            }
        }

        private static void SpawnProp(Transform parent, int resourceIndex, int count)
        {
            bool food = resourceIndex == ResourceInfo.FoodIndex;
            // Food is a roast (Kenney), minerals share one crystal mesh, each in its
            // legend color. Models come out of ModelLibrary 1 unit tall, base at origin.
            GameObject prop = ModelLibrary.Spawn(food ? "Food" : "Crystal", parent, ResourceInfo.Colors[resourceIndex]);
            prop.name = ResourceInfo.Names[resourceIndex];

            // Bigger stacks read bigger, like the browser's sizeIndex sprites.
            float size = (food ? 0.14f : 0.16f) + 0.04f * Mathf.Min(count, 4);
            Vector2 slot = Slots[resourceIndex];
            prop.transform.localScale = Vector3.one * size;
            prop.transform.localPosition = new Vector3(slot.x, 0.05f, slot.y);
            // Scatter the yaw a little so tiles do not look copy-pasted.
            prop.transform.localRotation = Quaternion.Euler(0f, (resourceIndex * 53 + count * 31) % 360, 0f);
        }

        private void RefreshTints()
        {
            if (_width == 0 || _height == 0)
                return;
            for (int x = 0; x < _width; x++)
            {
                for (int y = 0; y < _height; y++)
                {
                    Color color = (x + y) % 2 == 0 ? GrassA : GrassB;
                    if (_state.TileAt(x, y).Incanting)
                    {
                        // Quantized pulse so the material cache holds a handful of
                        // shades instead of one material per frame.
                        float pulse = Mathf.Round((0.55f + 0.25f * Mathf.Sin(Time.time * 6f)) * 8f) / 8f;
                        color = Color.Lerp(color, IncantColor, pulse);
                    }
                    else if (SelectedTile.x == x && SelectedTile.y == y)
                        color = SelectedColor;
                    else if (HoveredTile.x == x && HoveredTile.y == y)
                        color = HoverColor;
                    _tileRenderers[x, y].sharedMaterial = MaterialCache.GetLit(color);
                }
            }
        }
    }
}
