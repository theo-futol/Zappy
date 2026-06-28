#version 330 core
// Vertex shader — s'exécute une fois par sommet.
// Entrée  : la position du sommet, sur l'attribut location 0 (le slot que le VAO branchera).
// Sortie  : gl_Position (position finale en clip space).
// Étape 1 : pass-through en NDC (pas encore de matrices). Les matrices viendront à l'étape 2.
//
// TODO (Nathaniel) : écris le GLSL ici.
layout (location = 0) in vec3 aPos;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

void main()
{
    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
}