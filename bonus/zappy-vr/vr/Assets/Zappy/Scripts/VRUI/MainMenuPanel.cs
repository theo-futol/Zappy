using System;
using UnityEngine;

namespace Zappy.VRUI
{
    /// <summary>
    /// World-space configuration menu: bridge host and port fields (edited with the
    /// VR keyboard), a connect button and a quit button, plus an error line for
    /// failed connections.
    /// </summary>
    public class MainMenuPanel : MonoBehaviour
    {
        private enum Field
        {
            Host,
            Port
        }

        /// <summary>Called with the validated host and port when CONNECT is pressed.</summary>
        public Action<string, int> OnConnect;

        public Action OnQuit;

        private string _host = "localhost";
        private string _port = "8081";
        private Field _focus = Field.Host;
        private TextMesh _hostValue;
        private TextMesh _portValue;
        private TextMesh _errorLabel;
        private VRButton _hostButton;
        private VRButton _portButton;

        private static readonly Color FieldColor = new Color(0.16f, 0.17f, 0.22f);
        private static readonly Color FocusColor = new Color(0.30f, 0.33f, 0.45f);
        private static readonly Color ErrorColor = new Color(1.0f, 0.35f, 0.30f);

        public void Build()
        {
            UIFactory.Panel(transform, "Backdrop", 1.0f, 1.05f, UIFactory.PanelColor);

            UIFactory.Label(transform, "ZAPPY BRIDGE", new Vector3(0f, 0.44f, 0f), 0.07f, UIFactory.AccentColor);

            UIFactory.Label(transform, "HOST", new Vector3(-0.42f, 0.335f, 0f), 0.035f, UIFactory.AccentColor, TextAnchor.MiddleLeft);
            _hostButton = UIFactory.Button(transform, "", new Vector3(0f, 0.27f, 0f), 0.84f, 0.075f, () => SetFocus(Field.Host), FieldColor);
            _hostValue = UIFactory.Label(_hostButton.transform, _host, new Vector3(-0.39f, 0f, -0.005f), 0.04f, UIFactory.LabelColor, TextAnchor.MiddleLeft);

            UIFactory.Label(transform, "PORT", new Vector3(-0.42f, 0.185f, 0f), 0.035f, UIFactory.AccentColor, TextAnchor.MiddleLeft);
            _portButton = UIFactory.Button(transform, "", new Vector3(0f, 0.12f, 0f), 0.84f, 0.075f, () => SetFocus(Field.Port), FieldColor);
            _portValue = UIFactory.Label(_portButton.transform, _port, new Vector3(-0.39f, 0f, -0.005f), 0.04f, UIFactory.LabelColor, TextAnchor.MiddleLeft);

            var keyboardHolder = new GameObject("Keyboard");
            keyboardHolder.transform.SetParent(transform, false);
            keyboardHolder.transform.localPosition = new Vector3(0f, 0.02f, 0f);
            var keyboard = keyboardHolder.AddComponent<VRKeyboard>();
            keyboard.OnKey = AppendChar;
            keyboard.OnBackspace = Backspace;
            keyboard.OnClear = Clear;
            keyboard.Build();

            UIFactory.Button(transform, "CONNECT", new Vector3(-0.20f, -0.38f, 0f), 0.36f, 0.08f, Validate, UIFactory.AccentColor);
            UIFactory.Button(transform, "QUIT", new Vector3(0.20f, -0.38f, 0f), 0.36f, 0.08f, () => OnQuit?.Invoke());

            _errorLabel = UIFactory.Label(transform, "", new Vector3(0f, -0.47f, 0f), 0.032f, ErrorColor);
            SetFocus(Field.Host);
        }

        public void SetDefaults(string host, int port)
        {
            if (!string.IsNullOrEmpty(host))
                _host = host;
            if (port > 0)
                _port = port.ToString();
            Refresh();
        }

        public void SetError(string error)
        {
            if (_errorLabel != null)
                _errorLabel.text = error ?? "";
        }

        private void SetFocus(Field field)
        {
            _focus = field;
            _hostButton.SetBaseColor(field == Field.Host ? FocusColor : FieldColor);
            _portButton.SetBaseColor(field == Field.Port ? FocusColor : FieldColor);
        }

        private void AppendChar(char key)
        {
            if (_focus == Field.Host)
            {
                if (_host.Length < 40)
                    _host += key;
            }
            else if (key >= '0' && key <= '9' && _port.Length < 5)
            {
                _port += key;
            }
            Refresh();
        }

        private void Backspace()
        {
            if (_focus == Field.Host && _host.Length > 0)
                _host = _host.Substring(0, _host.Length - 1);
            else if (_focus == Field.Port && _port.Length > 0)
                _port = _port.Substring(0, _port.Length - 1);
            Refresh();
        }

        private void Clear()
        {
            if (_focus == Field.Host)
                _host = "";
            else
                _port = "";
            Refresh();
        }

        private void Refresh()
        {
            if (_hostValue != null)
                _hostValue.text = _host;
            if (_portValue != null)
                _portValue.text = _port;
        }

        private void Validate()
        {
            if (_host.Length == 0)
            {
                SetError("Host is required");
                return;
            }
            if (!int.TryParse(_port, out int port) || port < 1 || port > 65535)
            {
                SetError("Port must be 1-65535");
                return;
            }
            SetError("");
            OnConnect?.Invoke(_host, port);
        }
    }
}
