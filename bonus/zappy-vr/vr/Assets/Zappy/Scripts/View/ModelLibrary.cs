using UnityEngine;

namespace Zappy.View
{
    /// <summary>
    /// Loads the CC0 models from Resources/Models (see CREDITS.txt) and hands them
    /// out normalized: exactly 1 unit tall with the base at the holder's origin, so
    /// callers just scale the holder to the height they want. All materials are
    /// replaced with our URP cache materials — imported ones may reference shaders
    /// or textures that do not survive a Quest build.
    /// </summary>
    public static class ModelLibrary
    {
        /// <summary>A normalized model instance, or a primitive fallback if missing.</summary>
        public static GameObject Spawn(string modelName, Transform parent, Color tint)
        {
            var holder = new GameObject(modelName);
            holder.transform.SetParent(parent, false);

            GameObject prefab = Resources.Load<GameObject>("Models/" + modelName);
            if (prefab == null)
            {
                Debug.LogError($"[Zappy] Model '{modelName}' missing from Resources/Models, using a cube");
                GameObject cube = GameObject.CreatePrimitive(PrimitiveType.Cube);
                Object.Destroy(cube.GetComponent<Collider>());
                cube.transform.SetParent(holder.transform, false);
                cube.transform.localPosition = new Vector3(0f, 0.5f, 0f);
                Tint(holder, tint);
                return holder;
            }

            // Instantiate detached at the origin so renderer bounds are measured in
            // a clean identity frame, then normalize and attach.
            GameObject instance = Object.Instantiate(prefab);
            instance.transform.SetPositionAndRotation(Vector3.zero, Quaternion.identity);
            foreach (Animator animator in instance.GetComponentsInChildren<Animator>(true))
                Object.Destroy(animator);
            foreach (Animation animation in instance.GetComponentsInChildren<Animation>(true))
                Object.Destroy(animation);

            Bounds bounds = MeasureBounds(instance);
            float height = Mathf.Max(bounds.size.y, 0.001f);
            float k = 1f / height;
            Debug.Log($"[Zappy] Model '{modelName}': raw height {height:F3}, normalize x{k:F3}");
            instance.transform.localScale *= k;
            instance.transform.SetParent(holder.transform, false);
            // Center the footprint and sit the base on the holder origin.
            instance.transform.localPosition =
                new Vector3(-bounds.center.x * k, -bounds.min.y * k, -bounds.center.z * k);

            Tint(holder, tint);
            return holder;
        }

        /// <summary>Swap every renderer's material for a cached lit one in this color.</summary>
        public static void Tint(GameObject root, Color color)
        {
            Material material = MaterialCache.GetLit(color);
            foreach (Renderer renderer in root.GetComponentsInChildren<Renderer>(true))
                renderer.sharedMaterial = material;
        }

        /// <summary>Swap every renderer's material for a cached transparent one (focus dim).</summary>
        public static void TintTransparent(GameObject root, Color color)
        {
            Material material = MaterialCache.GetUnlitTransparent(color);
            foreach (Renderer renderer in root.GetComponentsInChildren<Renderer>(true))
                renderer.sharedMaterial = material;
        }

        /// <summary>
        /// Combined bounds from mesh data (not renderer.bounds, which is unreliable
        /// for skinned meshes right after Instantiate). The instance sits at the
        /// world origin with identity rotation, so world space == model space here.
        /// </summary>
        private static Bounds MeasureBounds(GameObject instance)
        {
            var bounds = new Bounds(Vector3.zero, Vector3.zero);
            bool first = true;
            foreach (MeshFilter filter in instance.GetComponentsInChildren<MeshFilter>(true))
            {
                if (filter.sharedMesh != null)
                    Accumulate(ref bounds, ref first, filter.sharedMesh.bounds, filter.transform);
            }
            foreach (SkinnedMeshRenderer skinned in instance.GetComponentsInChildren<SkinnedMeshRenderer>(true))
            {
                if (skinned.sharedMesh != null)
                    Accumulate(ref bounds, ref first, skinned.sharedMesh.bounds, skinned.transform);
            }
            return first ? new Bounds(Vector3.zero, Vector3.one) : bounds;
        }

        private static void Accumulate(ref Bounds total, ref bool first, Bounds meshBounds, Transform owner)
        {
            // Walk the 8 corners through the transform so scaled/offset parts count fully.
            Vector3 extents = meshBounds.extents;
            for (int i = 0; i < 8; i++)
            {
                var corner = new Vector3(
                    meshBounds.center.x + ((i & 1) == 0 ? -extents.x : extents.x),
                    meshBounds.center.y + ((i & 2) == 0 ? -extents.y : extents.y),
                    meshBounds.center.z + ((i & 4) == 0 ? -extents.z : extents.z));
                Vector3 world = owner.TransformPoint(corner);
                if (first)
                {
                    total = new Bounds(world, Vector3.zero);
                    first = false;
                }
                else
                {
                    total.Encapsulate(world);
                }
            }
        }
    }
}
