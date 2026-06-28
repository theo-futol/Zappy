#version 330 core
// Vertex shader Phong — s'exécute une fois par sommet.
// Il transforme le sommet vers l'écran ET prépare les données dont le fragment
// shader a besoin pour l'éclairage : position monde, normale monde, coord de texture.

// Attributs du sommet (l'ordre = le layout {3, 3, 2} passé à Mesh::upload).
layout (location = 0) in vec3 aPos;    // position du sommet (espace modèle)
layout (location = 1) in vec3 aNormal; // normale du sommet (espace modèle)
layout (location = 2) in vec2 aUV;     // coordonnée de texture (U, V)
// Attribut PAR INSTANCE : matrice modèle -> monde propre à chaque copie dessinée.
// Une mat4 occupe 4 emplacements (3, 4, 5, 6) ; elle n'avance qu'une fois par instance.
layout (location = 3) in mat4 aInstanceModel;

uniform mat4 uView;         // monde -> caméra
uniform mat4 uProjection;   // caméra -> clip (perspective)
uniform mat3 uNormalMatrix; // transforme correctement les normales (voir .cpp appelant)

// Sorties interpolées, lues par phong.frag.
out vec3 FragPos;   // position du fragment dans le monde
out vec3 Normal;    // normale du fragment dans le monde
out vec2 TexCoord;  // coordonnée de texture

void main()
{
    vec4 worldPosition = aInstanceModel * vec4(aPos, 1.0);

    FragPos = vec3(worldPosition);          // position monde, pour les calculs de lumière
    Normal = uNormalMatrix * aNormal;       // normale réorientée dans le monde
    TexCoord = aUV;                          // transmise telle quelle
    gl_Position = uProjection * uView * worldPosition; // position finale à l'écran
}
