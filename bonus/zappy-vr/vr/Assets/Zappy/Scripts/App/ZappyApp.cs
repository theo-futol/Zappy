using UnityEngine;
using Zappy.VRUI;

namespace Zappy.App
{
    /// <summary>
    /// Entry point (the only scene component): builds the laser pointer and the main
    /// menu, then alternates menu and viewer sessions until the user quits. Connects
    /// to the Node bridge (bonus/GUI/bridge), not directly to zappy_server.
    /// </summary>
    public class ZappyApp : MonoBehaviour
    {
        // Fallback pose only; the menu is re-anchored in front of the user's actual
        // head as soon as tracking delivers a pose (see AnchorMenuWhenTracked).
        private static readonly Vector3 MenuAnchor = new Vector3(0f, 1.35f, 1.8f);
        private const float MenuDistance = 1.8f;

        [SerializeField] private string defaultHost = "localhost";
        [SerializeField] private int defaultBridgePort = 8081;

        private LaserPointer _laser;
        private MainMenuPanel _menu;
        private GameSession _session;

        private void Start()
        {
            gameObject.AddComponent<AmbientMusic>();
            PassthroughBackground.Enable();

            var laserHolder = new GameObject("LaserPointer");
            laserHolder.transform.SetParent(transform, false);
            _laser = laserHolder.AddComponent<LaserPointer>();

            var menuHolder = new GameObject("MainMenu");
            menuHolder.transform.SetParent(transform, false);
            menuHolder.transform.position = MenuAnchor;
            _menu = menuHolder.AddComponent<MainMenuPanel>();
            _menu.OnConnect = StartSession;
            _menu.OnQuit = Quit;
            _menu.Build();
            _menu.SetDefaults(defaultHost, defaultBridgePort);

            StartCoroutine(AnchorMenuWhenTracked());
            OVRManager.HMDMounted += OnHmdMounted;
        }

        private void OnDestroy()
        {
            OVRManager.HMDMounted -= OnHmdMounted;
        }

        /// <summary>
        /// With Build &amp; Run the app usually starts while the headset is on the desk,
        /// so the world origin (and a fixed menu position) can end up anywhere relative
        /// to the user. Wait for a live head pose, then place the menu in front of it.
        /// </summary>
        private System.Collections.IEnumerator AnchorMenuWhenTracked()
        {
            float deadline = Time.realtimeSinceStartup + 2f;
            while (Time.realtimeSinceStartup < deadline)
            {
                Camera head = Camera.main;
                if (head != null && head.transform.localPosition.sqrMagnitude > 0.01f)
                    break;
                yield return null;
            }
            AnchorInFrontOfUser(_menu.transform);
        }

        private void OnHmdMounted()
        {
            // The runtime recenters shortly after the headset is put on; re-anchor once
            // the new pose has settled, but only while the menu is up.
            if (_menu != null && _menu.gameObject.activeSelf)
                StartCoroutine(AnchorMenuWhenTracked());
        }

        private static void AnchorInFrontOfUser(Transform panel)
        {
            Camera head = Camera.main;
            if (head == null)
                return;
            Vector3 forward = head.transform.forward;
            forward.y = 0f;
            forward = forward.sqrMagnitude < 0.001f ? Vector3.forward : forward.normalized;
            Vector3 position = head.transform.position + forward * MenuDistance;
            position.y = Mathf.Max(head.transform.position.y - 0.25f, 0.2f);
            panel.SetPositionAndRotation(position, Quaternion.LookRotation(forward));
            Debug.Log($"[Zappy] Menu anchored at {position} (head at {head.transform.position})");
        }

        private void StartSession(string host, int port)
        {
            _menu.gameObject.SetActive(false);
            var sessionHolder = new GameObject("GameSession");
            sessionHolder.transform.SetParent(transform, false);
            _session = sessionHolder.AddComponent<GameSession>();
            _session.OnEnded = EndSession;
            _session.Begin(host, port, _laser);
        }

        private void EndSession(string error)
        {
            if (_session != null)
            {
                Destroy(_session.gameObject);
                _session = null;
            }
            _menu.gameObject.SetActive(true);
            _menu.SetError(error ?? "");
            AnchorInFrontOfUser(_menu.transform);
        }

        private static void Quit()
        {
#if UNITY_EDITOR
            UnityEditor.EditorApplication.isPlaying = false;
#else
            Application.Quit();
#endif
        }
    }
}
