#version 330 core
// Fragment shader "flash" — couleur unie avec opacité réglable.
// Sert au fondu blanc de la transition de menu (basic.frag reste opaque et partagé).
out vec4 FragColor;
uniform vec3 uColor;
uniform float uAlpha; // opacité du fondu (0 = invisible, 1 = plein)

void main()
{
    FragColor = vec4(uColor, uAlpha);
}
