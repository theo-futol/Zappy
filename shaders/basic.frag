#version 330 core
// Fragment shader — s'exécute une fois par fragment (pixel couvert).
// Entrée  : (rien pour l'instant ; plus tard, des valeurs interpolées depuis les sommets).
// Sortie  : la couleur du pixel (un out vec4).
// Étape 1 : une couleur unie suffit.
//
// TODO (Nathaniel) : écris le GLSL ici.
out vec4 FragColor;
uniform vec3 uColor;

void main()
{
    FragColor = vec4(uColor, 1.0);
}