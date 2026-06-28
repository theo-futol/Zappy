#version 330 core
// Vertex shader Phong — s'exécute une fois par sommet.
// Il transforme le sommet vers l'écran ET prépare les données dont le fragment
// shader a besoin pour l'éclairage : position monde, normale monde, coord de texture.

// Attributs du sommet (l'ordre = le layout {3, 3, 2, 4, 4} passé à Mesh::upload).
layout (location = 0) in vec3 aPos;     // position du sommet (espace modèle)
layout (location = 1) in vec3 aNormal;  // normale du sommet (espace modèle)
layout (location = 2) in vec2 aUV;      // coordonnée de texture (U, V)
layout (location = 3) in vec4 aJoints;  // 4 indices d'os (skinning ; 0 si non animé)
layout (location = 4) in vec4 aWeights; // 4 poids d'os (skinning ; 0 si non animé)
// Attribut PAR INSTANCE : matrice modèle -> monde propre à chaque copie dessinée.
// Fixé aux emplacements 8..11 (Mesh::InstanceBaseLocation), pour laisser 0..7 aux sommets.
layout (location = 8) in mat4 aInstanceModel;

uniform mat4 uView;         // monde -> caméra
uniform mat4 uProjection;   // caméra -> clip (perspective)
uniform mat3 uNormalMatrix; // transforme correctement les normales (voir .cpp appelant)

// Skinning : matrices d'os (model-space) et interrupteur. uSkinned=false => rendu statique.
const int MAX_JOINTS = 100;
uniform mat4 uJoints[MAX_JOINTS];
uniform bool uSkinned;

// Sorties interpolées, lues par phong.frag.
out vec3 FragPos;   // position du fragment dans le monde
out vec3 Normal;    // normale du fragment dans le monde
out vec2 TexCoord;  // coordonnée de texture

void main()
{
    mat4 skin = mat4(1.0);

    if (uSkinned)
    {
        // Mélange pondéré des 4 os : chaque sommet suit ses os selon ses poids.
        skin = aWeights.x * uJoints[int(aJoints.x)]
             + aWeights.y * uJoints[int(aJoints.y)]
             + aWeights.z * uJoints[int(aJoints.z)]
             + aWeights.w * uJoints[int(aJoints.w)];
    }

    vec4 localPosition = skin * vec4(aPos, 1.0);          // sommet posé par le squelette
    vec4 worldPosition = aInstanceModel * localPosition;  // puis placé dans le monde

    FragPos = vec3(worldPosition);                        // position monde, pour la lumière
    Normal = uNormalMatrix * mat3(skin) * aNormal;        // normale suivie par le skinning
    TexCoord = aUV;                                       // transmise telle quelle
    gl_Position = uProjection * uView * worldPosition;    // position finale à l'écran
}
