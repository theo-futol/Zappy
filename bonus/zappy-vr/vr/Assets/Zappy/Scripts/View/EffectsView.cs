using System.Collections.Generic;
using UnityEngine;
using Zappy.Model;

namespace Zappy.View
{
    /// <summary>
    /// Broadcast effects: a team-tinted speech bubble with the decoded message above
    /// the sender, plus expanding sound rings spreading from its tile over the map.
    /// </summary>
    public class EffectsView : MonoBehaviour
    {
        private const float BubbleLife = 8f;
        private const float FadeSpan = 1.5f;
        private const int MaxMessageLength = 60;
        private const float RippleLife = 1.6f;
        private const float RippleStartRadius = 0.35f;
        private const float RippleMaxRadius = 2.6f;
        private const int RingsPerBroadcast = 2;
        private const float RingStagger = 0.35f;

        private static readonly Color RippleColor = new Color(1.0f, 0.85f, 0.4f, 0.85f);
        private static Mesh _ringMesh;

        private class Bubble
        {
            public GameObject Root;
            public TextMesh Text;
            public Renderer Background;
            public Color BackgroundColor;
            public float Age;
        }

        private class Ripple
        {
            public GameObject Visual;
            public Material Material;
            public float Age;
        }

        private GameState _state;
        private EntitiesView _entities;
        private readonly List<Bubble> _bubbles = new();
        private readonly List<Ripple> _ripples = new();
        private long _lastSequence = -1;

        public void Bind(GameState state, EntitiesView entities)
        {
            _state = state;
            _entities = entities;
            _lastSequence = -1;
        }

        private void Update()
        {
            if (_state == null)
                return;
            foreach (BroadcastEntry broadcast in _state.Broadcasts)
            {
                if (broadcast.Sequence <= _lastSequence)
                    continue;
                _lastSequence = broadcast.Sequence;
                SpawnBubble(broadcast);
                SpawnRipples(broadcast);
            }
            AgeBubbles();
            AgeRipples();
        }

        /// <summary>The sound circles spreading around the shouting trantorian.</summary>
        private void SpawnRipples(BroadcastEntry broadcast)
        {
            if (!_state.Players.TryGetValue(broadcast.PlayerId, out PlayerData sender))
                return;
            for (int ring = 0; ring < RingsPerBroadcast; ring++)
            {
                var visual = new GameObject("BroadcastRipple");
                visual.transform.SetParent(transform, false);
                visual.transform.localPosition = MapView.TileCenter(sender.X, sender.Y) + new Vector3(0f, 0.14f, 0f);
                visual.AddComponent<MeshFilter>().sharedMesh = _ringMesh ??= RingMesh(48, 0.72f);
                var renderer = visual.AddComponent<MeshRenderer>();
                var material = new Material(MaterialCache.GetUnlitTransparent(RippleColor));
                renderer.material = material;
                visual.SetActive(ring == 0);
                _ripples.Add(new Ripple { Visual = visual, Material = material, Age = -ring * RingStagger });
            }
        }

        private void AgeRipples()
        {
            for (int i = _ripples.Count - 1; i >= 0; i--)
            {
                Ripple ripple = _ripples[i];
                ripple.Age += Time.deltaTime;
                if (ripple.Age < 0f)
                    continue;
                float progress = ripple.Age / RippleLife;
                if (progress >= 1f || ripple.Visual == null)
                {
                    if (ripple.Visual != null)
                        Destroy(ripple.Visual);
                    _ripples.RemoveAt(i);
                    continue;
                }
                if (!ripple.Visual.activeSelf)
                    ripple.Visual.SetActive(true);
                float diameter = Mathf.Lerp(RippleStartRadius, RippleMaxRadius, progress) * 2f;
                ripple.Visual.transform.localScale = new Vector3(diameter, 1f, diameter);
                Color color = RippleColor;
                color.a = (1f - progress) * RippleColor.a;
                ripple.Material.SetColor("_BaseColor", color);
            }
        }

        /// <summary>Flat ring (annulus) in the XZ plane, unit outer diameter, facing up.</summary>
        private static Mesh RingMesh(int segments, float innerRatio)
        {
            var vertices = new List<Vector3>();
            var triangles = new List<int>();
            for (int i = 0; i <= segments; i++)
            {
                float angle = i * 2f * Mathf.PI / segments;
                var direction = new Vector3(Mathf.Cos(angle), 0f, Mathf.Sin(angle));
                vertices.Add(direction * 0.5f);
                vertices.Add(direction * (0.5f * innerRatio));
            }
            for (int i = 0; i < segments; i++)
            {
                int outer0 = i * 2, inner0 = i * 2 + 1, outer1 = i * 2 + 2, inner1 = i * 2 + 3;
                triangles.AddRange(new[] { outer0, inner0, outer1, inner0, inner1, outer1 });
            }
            var mesh = new Mesh { name = "BroadcastRing" };
            mesh.SetVertices(vertices);
            mesh.SetTriangles(triangles, 0);
            mesh.RecalculateNormals();
            mesh.RecalculateBounds();
            return mesh;
        }

        private void SpawnBubble(BroadcastEntry broadcast)
        {
            Transform player = _entities.PlayerTransform(broadcast.PlayerId);
            if (player == null)
                return;

            string text = broadcast.Message.Length > MaxMessageLength
                ? broadcast.Message.Substring(0, MaxMessageLength) + "…"
                : broadcast.Message;
            int level = _state.Players.TryGetValue(broadcast.PlayerId, out PlayerData data) ? data.Level : 1;
            Color teamColor = TeamColors.Get(broadcast.Team);

            var root = new GameObject("SpeechBubble");
            root.transform.SetParent(player, false);
            // Above the beacon pillar (which tops out around HeadHeight + 0.55).
            root.transform.localPosition = new Vector3(0f, EntitiesView.HeadHeight(level) + 0.68f, 0f);
            root.AddComponent<Billboard>();

            var label = new GameObject("Text");
            label.transform.SetParent(root.transform, false);
            label.transform.localPosition = new Vector3(0f, 0f, -0.01f);
            var mesh = label.AddComponent<TextMesh>();
            mesh.font = UiFontProvider.Font;
            label.GetComponent<MeshRenderer>().material = UiFontProvider.Font.material;
            mesh.fontSize = 48;
            mesh.characterSize = 0.09f * 10f / mesh.fontSize;
            mesh.anchor = TextAnchor.MiddleCenter;
            mesh.alignment = TextAlignment.Center;
            mesh.color = Color.white;
            mesh.text = text;

            // Team-tinted backdrop sized to the text.
            GameObject backdrop = GameObject.CreatePrimitive(PrimitiveType.Quad);
            backdrop.name = "Backdrop";
            Destroy(backdrop.GetComponent<Collider>());
            backdrop.transform.SetParent(root.transform, false);
            float width = Mathf.Clamp(0.055f * text.Length, 0.4f, 3.4f);
            backdrop.transform.localScale = new Vector3(width, 0.22f, 1f);
            var backgroundColor = new Color(teamColor.r, teamColor.g, teamColor.b, 0.35f);
            backdrop.GetComponent<Renderer>().material =
                new Material(MaterialCache.GetUnlitTransparent(backgroundColor));

            _bubbles.Add(new Bubble
            {
                Root = root,
                Text = mesh,
                Background = backdrop.GetComponent<Renderer>(),
                BackgroundColor = backgroundColor,
                Age = 0f
            });
        }

        private void AgeBubbles()
        {
            for (int i = _bubbles.Count - 1; i >= 0; i--)
            {
                Bubble bubble = _bubbles[i];
                bubble.Age += Time.deltaTime;
                if (bubble.Age >= BubbleLife || bubble.Root == null)
                {
                    if (bubble.Root != null)
                        Destroy(bubble.Root);
                    _bubbles.RemoveAt(i);
                    continue;
                }
                float alpha = Mathf.Clamp01((BubbleLife - bubble.Age) / FadeSpan);
                Color textColor = bubble.Text.color;
                textColor.a = alpha;
                bubble.Text.color = textColor;
                Color backgroundColor = bubble.BackgroundColor;
                backgroundColor.a *= alpha;
                bubble.Background.material.SetColor("_BaseColor", backgroundColor);
            }
        }
    }
}
