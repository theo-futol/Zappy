using System;
using UnityEngine;

namespace Zappy.VRUI
{
    /// <summary>
    /// A compact world-space keyboard for the menu fields (host names, ports):
    /// letters, digits, dot and dash, plus backspace and clear.
    /// </summary>
    public class VRKeyboard : MonoBehaviour
    {
        public Action<char> OnKey;
        public Action OnBackspace;
        public Action OnClear;

        private const float KeySize = 0.055f;
        private const float KeyStep = 0.062f;

        private static readonly string[] Rows =
        {
            "1234567890",
            "qwertyuiop",
            "asdfghjkl-",
            "zxcvbnm._"
        };

        public void Build()
        {
            for (int row = 0; row < Rows.Length; row++)
            {
                string keys = Rows[row];
                float rowWidth = keys.Length * KeyStep;
                for (int i = 0; i < keys.Length; i++)
                {
                    char key = keys[i];
                    var position = new Vector3(-rowWidth / 2f + KeyStep * (i + 0.5f), -row * KeyStep, 0f);
                    UIFactory.Button(transform, key.ToString(), position, KeySize, KeySize, () => OnKey?.Invoke(key));
                }
            }

            float bottomY = -Rows.Length * KeyStep;
            UIFactory.Button(transform, "DEL", new Vector3(-0.10f, bottomY, 0f), 0.14f, KeySize, () => OnBackspace?.Invoke());
            UIFactory.Button(transform, "CLR", new Vector3(0.10f, bottomY, 0f), 0.14f, KeySize, () => OnClear?.Invoke());
        }
    }
}
