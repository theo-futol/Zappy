using System.Collections.Generic;
using UnityEngine;

namespace Zappy.View
{
    /// <summary>Shared URP materials keyed by color, so the whole scene reuses a few instances.</summary>
    public static class MaterialCache
    {
        private static readonly Dictionary<Color, Material> Lit = new();
        private static readonly Dictionary<Color, Material> Unlit = new();

        // Base materials shipped in Resources so the URP shaders (and the transparent
        // variant) are never stripped from device builds; Shader.Find alone only works
        // when some built asset already references the shader.
        private static Material _litBase;
        private static Material _unlitBase;

        public static Material GetLit(Color color)
        {
            if (Lit.TryGetValue(color, out Material cached))
                return cached;
            var material = new Material(BaseMaterial(ref _litBase, "ZappyLit", "Universal Render Pipeline/Lit"));
            material.SetColor("_BaseColor", color);
            Lit[color] = material;
            return material;
        }

        /// <summary>Unlit transparent material (UI quads, highlights, dimmed entities).</summary>
        public static Material GetUnlitTransparent(Color color)
        {
            if (Unlit.TryGetValue(color, out Material cached))
                return cached;
            var material = new Material(BaseMaterial(ref _unlitBase, "ZappyUnlitTransparent", "Universal Render Pipeline/Unlit"));
            material.SetColor("_BaseColor", color);
            material.SetFloat("_Surface", 1f); // Transparent
            material.SetFloat("_Blend", 0f);   // Alpha blend
            material.SetOverrideTag("RenderType", "Transparent");
            material.SetInt("_SrcBlend", (int)UnityEngine.Rendering.BlendMode.SrcAlpha);
            material.SetInt("_DstBlend", (int)UnityEngine.Rendering.BlendMode.OneMinusSrcAlpha);
            material.SetInt("_ZWrite", 0);
            material.renderQueue = (int)UnityEngine.Rendering.RenderQueue.Transparent;
            Unlit[color] = material;
            return material;
        }

        private static Material BaseMaterial(ref Material cache, string resourceName, string shaderName)
        {
            if (cache != null)
                return cache;
            cache = Resources.Load<Material>(resourceName);
            if (cache == null)
            {
                Debug.LogError($"[Zappy] Base material '{resourceName}' missing from Resources, falling back to Shader.Find(\"{shaderName}\")");
                Shader shader = Shader.Find(shaderName);
                if (shader == null)
                {
                    Debug.LogError($"[Zappy] Shader '{shaderName}' is not in the build; UI will be invisible");
                    shader = Shader.Find("Hidden/InternalErrorShader");
                }
                cache = new Material(shader);
            }
            return cache;
        }
    }
}
