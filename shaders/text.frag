#version 330 core
// Fragment shader du texte — la texture du glyphe est mono-canal (GL_RED).
// Son unique valeur (.r) est la COUVERTURE du glyphe : on l'utilise comme alpha,
// et on teinte avec uTextColor. Le blending (déjà actif) fait l'anti-aliasing des bords.
in vec2 TexCoords;

out vec4 FragColor;

uniform sampler2D uText;
uniform vec3 uTextColor;

void main()
{
    float coverage = texture(uText, TexCoords).r;
    FragColor = vec4(uTextColor, coverage);
}
