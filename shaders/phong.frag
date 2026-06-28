#version 330 core
// Fragment shader Phong — s'exécute une fois par pixel couvert.
// Il combine trois contributions d'une lumière (ambiant + diffus + spéculaire)
// et la couleur de la texture pour produire la couleur finale du pixel.

// Entrées interpolées depuis phong.vert (une valeur par pixel).
in vec3 FragPos;   // position du fragment dans le monde
in vec3 Normal;    // normale du fragment dans le monde (à renormaliser)
in vec2 TexCoord;  // coordonnée de texture

uniform sampler2D uTexture; // la texture liée (slot de Texture::bind)
uniform int uHasTexture;    // 1 = échantillonner uTexture ; 0 = utiliser uBaseColor
uniform vec3 uBaseColor;    // couleur de repli quand il n'y a pas de texture
uniform vec3 uLightPos;     // position de la lampe (monde)
uniform vec3 uViewPos;      // position de la caméra (monde)
uniform vec3 uLightColor;   // couleur/intensité de la lampe

out vec4 FragColor;

// Réglages de matériau (constants : mêmes pour tout objet en v1).
const float AmbientStrength = 0.45;  // niveau de lumière de fond (haut = facettes moins contrastées)
const float SpecularStrength = 0.12; // intensité du reflet (bas = pas de highlights durs qui révèlent les facettes)
const float Shininess = 32.0;        // taille du reflet (plus grand = plus petit/net)

void main()
{
    vec3 normal = normalize(Normal);                 // longueur remise à 1 après interpolation
    vec3 lightDir = normalize(uLightPos - FragPos);  // du fragment vers la lampe
    vec3 viewDir = normalize(uViewPos - FragPos);    // du fragment vers la caméra
    vec3 reflectDir = reflect(-lightDir, normal);    // rayon réfléchi sur la surface

    vec3 ambient = AmbientStrength * uLightColor;
    vec3 diffuse = max(dot(normal, lightDir), 0.0) * uLightColor;
    vec3 specular = SpecularStrength * pow(max(dot(viewDir, reflectDir), 0.0), Shininess) * uLightColor;

    vec3 albedo = (uHasTexture != 0) ? texture(uTexture, TexCoord).rgb : uBaseColor;
    vec3 result = (ambient + diffuse + specular) * albedo;

    FragColor = vec4(result, 1.0);
}
