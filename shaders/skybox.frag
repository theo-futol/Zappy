#version 330 core
// Fragment shader skybox — échantillonne le cube map dans la direction du sommet.

in vec3 TexDir;          // direction du ciel (interpolée)
uniform samplerCube uSky; // le cube map (6 faces)

out vec4 FragColor;

void main()
{
    FragColor = texture(uSky, TexDir); // couleur du ciel dans cette direction
}
