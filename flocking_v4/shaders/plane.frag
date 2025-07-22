#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform vec3 lightDir = normalize(vec3(1.0, -1.0, 1.0));
uniform vec3 lightColor = vec3(1.0);
uniform vec3 objectColor = vec3(0.35, 0.7, 0.35);

void main() {
    float diff = max(dot(Normal, -lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    vec3 result = (diffuse + vec3(0.2)) * objectColor;
    FragColor = vec4(result, 1.0);
}