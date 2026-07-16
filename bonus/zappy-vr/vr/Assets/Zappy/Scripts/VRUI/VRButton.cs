using System;
using UnityEngine;

namespace Zappy.VRUI
{
    /// <summary>
    /// A clickable 3D button: a colored quad with a collider and a text label.
    /// The laser pointer drives hover/click through SetHover and Click.
    /// </summary>
    public class VRButton : MonoBehaviour
    {
        public Action OnClick;

        private Renderer _background;
        private Color _baseColor;
        private bool _hovered;

        public void Init(Renderer background, Color baseColor)
        {
            _background = background;
            _baseColor = baseColor;
            Apply();
        }

        public void SetBaseColor(Color color)
        {
            _baseColor = color;
            Apply();
        }

        public void SetHover(bool hovered)
        {
            if (_hovered == hovered)
                return;
            _hovered = hovered;
            Apply();
        }

        public void Click()
        {
            OnClick?.Invoke();
        }

        private void Apply()
        {
            if (_background == null)
                return;
            Color color = _hovered ? Color.Lerp(_baseColor, Color.white, 0.35f) : _baseColor;
            _background.material.SetColor("_BaseColor", color);
        }
    }
}
