using System;
using UnityEngine;
using Zappy.View;

namespace Zappy.VRUI
{
    /// <summary>
    /// Builds world-space UI pieces from primitives and legacy TextMesh labels
    /// (no TextMeshPro/uGUI dependency). All sizes are in local units of the
    /// parent panel; panels face +Z.
    /// </summary>
    public static class UIFactory
    {
        public static readonly Color PanelColor = new Color(0.10f, 0.10f, 0.14f, 0.92f);
        public static readonly Color AccentColor = new Color(0.95f, 0.55f, 0.15f);
        public static readonly Color ButtonColor = new Color(0.22f, 0.24f, 0.32f);
        public static readonly Color LabelColor = new Color(0.92f, 0.92f, 0.95f);

        /// <summary>A flat backdrop quad (no collider) of the given size.</summary>
        public static GameObject Panel(Transform parent, string name, float width, float height, Color color)
        {
            var panel = new GameObject(name);
            panel.transform.SetParent(parent, false);

            GameObject backdrop = GameObject.CreatePrimitive(PrimitiveType.Quad);
            backdrop.name = "Backdrop";
            UnityEngine.Object.Destroy(backdrop.GetComponent<Collider>());
            backdrop.transform.SetParent(panel.transform, false);
            backdrop.transform.localScale = new Vector3(width, height, 1f);
            backdrop.GetComponent<Renderer>().sharedMaterial = MaterialCache.GetUnlitTransparent(color);
            return panel;
        }

        /// <summary>A small colored quad (legend/tile-info icons).</summary>
        public static GameObject Icon(Transform parent, Vector3 position, float size, Color color)
        {
            GameObject icon = GameObject.CreatePrimitive(PrimitiveType.Quad);
            icon.name = "Icon";
            UnityEngine.Object.Destroy(icon.GetComponent<Collider>());
            icon.transform.SetParent(parent, false);
            icon.transform.localPosition = position + new Vector3(0f, 0f, -0.002f);
            icon.transform.localScale = new Vector3(size, size, 1f);
            icon.GetComponent<Renderer>().sharedMaterial = MaterialCache.GetUnlitTransparent(color);
            return icon;
        }

        /// <summary>A text label. Anchor is the local position of the text pivot on the parent.</summary>
        public static TextMesh Label(Transform parent, string text, Vector3 position, float size, Color color, TextAnchor anchor = TextAnchor.MiddleCenter)
        {
            var holder = new GameObject("Label");
            holder.transform.SetParent(parent, false);
            holder.transform.localPosition = position + new Vector3(0f, 0f, -0.002f);
            holder.transform.localRotation = Quaternion.identity;

            var label = holder.AddComponent<TextMesh>();
            label.text = text;
            label.font = UiFontProvider.Font;
            holder.GetComponent<MeshRenderer>().material = UiFontProvider.Font.material;
            label.fontSize = 64;
            label.characterSize = size * 10f / label.fontSize;
            label.anchor = anchor;
            label.alignment = anchor == TextAnchor.MiddleLeft ? TextAlignment.Left : TextAlignment.Center;
            label.color = color;
            return label;
        }

        /// <summary>A clickable button quad with a centered label and a BoxCollider.</summary>
        public static VRButton Button(Transform parent, string text, Vector3 position, float width, float height, Action onClick, Color? color = null)
        {
            Color baseColor = color ?? ButtonColor;
            var holder = new GameObject($"Button_{text}");
            holder.transform.SetParent(parent, false);
            holder.transform.localPosition = position;

            GameObject face = GameObject.CreatePrimitive(PrimitiveType.Quad);
            face.name = "Face";
            UnityEngine.Object.Destroy(face.GetComponent<Collider>());
            face.transform.SetParent(holder.transform, false);
            face.transform.localScale = new Vector3(width, height, 1f);
            var renderer = face.GetComponent<Renderer>();
            renderer.material = new Material(MaterialCache.GetUnlitTransparent(baseColor));

            var collider = holder.AddComponent<BoxCollider>();
            collider.size = new Vector3(width, height, 0.02f);

            Label(holder.transform, text, new Vector3(0f, 0f, -0.005f), height * 0.55f, LabelColor);

            var button = holder.AddComponent<VRButton>();
            button.Init(renderer, baseColor);
            button.OnClick = onClick;
            return button;
        }
    }
}
