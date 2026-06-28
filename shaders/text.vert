#version 330 core
// Vertex shader du texte — un sommet = un coin de quad de glyphe.
// Entrée : vertex.xy = position écran (pixels), vertex.zw = coordonnée de texture.
// On projette en clip space avec une ortho écran (origine bas-gauche).
layout (location = 0) in vec4 vertex;

out vec2 TexCoords;

uniform mat4 uProjection;

void main()
{
    gl_Position = uProjection * vec4(vertex.xy, 0.0, 1.0);
    TexCoords = vertex.zw;
}
