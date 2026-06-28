#version 330 core
// Vertex shader skybox — dessine le ciel autour de la caméra.
// La position du sommet du cube sert DIRECTEMENT de direction d'échantillonnage
// dans le cube map : un sommet à (1, 1, -1) regarde vers ce coin du ciel.

layout (location = 0) in vec3 aPos; // sommet du cube unité (sert de direction)

uniform mat4 uView;       // vue SANS translation (rotation seule) → ciel infini
uniform mat4 uProjection; // perspective

out vec3 TexDir; // direction du ciel, interpolée et lue par le fragment shader

void main()
{
    TexDir = aPos;                                       // direction = position du cube
    gl_Position = uProjection * uView * vec4(aPos, 1.0); // place le cube à l'écran
}
