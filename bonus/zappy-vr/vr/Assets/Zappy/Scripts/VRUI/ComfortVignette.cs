using System.Collections.Generic;
using UnityEngine;
using Zappy.View;

namespace Zappy.VRUI
{
    /// <summary>
    /// Anti-motion-sickness tunneling vignette (the classic VR "comfort vignette"):
    /// whenever the world moves under the user — trantorian POV, joystick locomotion —
    /// the edges of the view fade to black, keeping a clear circle in the center.
    /// Built as head-locked concentric rings of increasing opacity, faded in and out
    /// smoothly. Turn the whole feature off with ComfortVignetteEnabled = false.
    /// </summary>
    public class ComfortVignette : MonoBehaviour
    {
        /// <summary>Master switch for the comfort vignette.</summary>
        public static bool ComfortVignetteEnabled = true;

        private const float FadeSpeed = 6f;
        private const int Segments = 48;

        // Ring boundaries (meters at 0.45 m from the eye) and their target opacities:
        // clear center out to ~30° of view, solid black past ~50°.
        private static readonly float[] Radii = { 0.26f, 0.33f, 0.40f, 0.48f, 0.56f, 1.6f };
        private static readonly float[] Alphas = { 0.18f, 0.45f, 0.70f, 0.88f, 0.97f };

        private GameObject _root;
        private readonly List<Material> _materials = new();
        private float _intensity;
        private bool _engaged;

        /// <summary>Call each frame: true while artificial motion is happening.</summary>
        public void SetEngaged(bool engaged) => _engaged = engaged && ComfortVignetteEnabled;

        private void Update()
        {
            float target = _engaged ? 1f : 0f;
            _intensity = Mathf.MoveTowards(_intensity, target, Time.deltaTime * FadeSpeed);

            if (_intensity <= 0.01f)
            {
                if (_root != null && _root.activeSelf)
                    _root.SetActive(false);
                return;
            }
            if (_root == null)
            {
                Build();
                if (_root == null)
                    return; // No camera yet; retry next frame.
            }
            if (!_root.activeSelf)
                _root.SetActive(true);
            for (int i = 0; i < _materials.Count; i++)
                _materials[i].SetColor("_BaseColor", new Color(0f, 0f, 0f, Alphas[i] * _intensity));
        }

        private void Build()
        {
            Camera head = Camera.main;
            if (head == null)
                return;
            _root = new GameObject("ComfortVignette");
            _root.transform.SetParent(head.transform, false);
            _root.transform.localPosition = new Vector3(0f, 0f, 0.45f);

            for (int i = 0; i < Alphas.Length; i++)
            {
                var ring = new GameObject($"Ring{i}");
                ring.transform.SetParent(_root.transform, false);
                ring.AddComponent<MeshFilter>().sharedMesh = RingMesh(Radii[i], Radii[i + 1]);
                var renderer = ring.AddComponent<MeshRenderer>();
                var material = new Material(MaterialCache.GetUnlitTransparent(new Color(0f, 0f, 0f, Alphas[i])));
                material.renderQueue = 4500; // Draw over the world and the UI.
                renderer.material = material;
                _materials.Add(material);
            }
        }

        /// <summary>Flat annulus in the XY plane, facing the camera (-Z side visible).</summary>
        private static Mesh RingMesh(float inner, float outer)
        {
            var vertices = new List<Vector3>();
            var triangles = new List<int>();
            for (int i = 0; i <= Segments; i++)
            {
                float angle = i * 2f * Mathf.PI / Segments;
                var direction = new Vector3(Mathf.Cos(angle), Mathf.Sin(angle), 0f);
                vertices.Add(direction * outer);
                vertices.Add(direction * inner);
            }
            for (int i = 0; i < Segments; i++)
            {
                int outer0 = i * 2, inner0 = i * 2 + 1, outer1 = i * 2 + 2, inner1 = i * 2 + 3;
                triangles.AddRange(new[] { outer0, inner0, outer1, inner0, inner1, outer1 });
            }
            var mesh = new Mesh { name = "VignetteRing" };
            mesh.SetVertices(vertices);
            mesh.SetTriangles(triangles, 0);
            mesh.RecalculateNormals();
            mesh.RecalculateBounds();
            return mesh;
        }

        private void OnDestroy()
        {
            // The rings live under the camera, not under this object.
            if (_root != null)
                Destroy(_root);
        }
    }
}
