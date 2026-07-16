using UnityEngine;

namespace Zappy.View
{
    /// <summary>Marks a tile GameObject so the laser pointer can resolve the grid cell it hit.</summary>
    public class TileMarker : MonoBehaviour
    {
        public int X;
        public int Y;
    }

    /// <summary>Marks an entity GameObject so the laser pointer can resolve which entity it hit.</summary>
    public class EntityMarker : MonoBehaviour
    {
        public string Type; // "player" or "egg"
        public int Number;
    }

    /// <summary>Keeps world-space labels readable by facing the camera every frame.</summary>
    public class Billboard : MonoBehaviour
    {
        private void LateUpdate()
        {
            Camera camera = Camera.main;
            if (camera == null)
                return;
            transform.rotation = Quaternion.LookRotation(transform.position - camera.transform.position);
        }
    }
}
