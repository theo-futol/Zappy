using UnityEngine;

namespace Zappy.App
{
    /// <summary>
    /// Mixed-reality background: the user's real room shows behind the virtual
    /// content instead of the skybox, everywhere — menu, tabletop and trantorian
    /// POV alike. Sets up an underlay passthrough layer and a transparent camera
    /// background at startup. SetRealWorld(false) can bring the skybox back if a
    /// fully virtual mode is ever wanted again.
    /// </summary>
    public static class PassthroughBackground
    {
        private static OVRPassthroughLayer _layer;

        public static void Enable()
        {
            if (OVRManager.instance != null)
                OVRManager.instance.isInsightPassthroughEnabled = true;
            if (_layer == null)
            {
                GameObject host = OVRManager.instance != null
                    ? OVRManager.instance.gameObject
                    : (Camera.main != null ? Camera.main.gameObject : null);
                if (host == null)
                    return;
                _layer = host.AddComponent<OVRPassthroughLayer>();
                _layer.overlayType = OVROverlay.OverlayType.Underlay;
            }
            SetRealWorld(true);
        }

        /// <summary>true: real room behind the game; false: virtual skybox (POV mode).</summary>
        public static void SetRealWorld(bool realWorld)
        {
            Camera head = Camera.main;
            if (head != null)
            {
                head.clearFlags = realWorld ? CameraClearFlags.SolidColor : CameraClearFlags.Skybox;
                head.backgroundColor = Color.clear;
            }
            if (_layer != null)
                _layer.hidden = !realWorld;
        }
    }
}
