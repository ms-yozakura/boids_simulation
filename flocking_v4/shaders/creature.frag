#version 330 core
out vec4 FragColor;

uniform vec3 creatureColor; // ← C++から送る

void main()
{
    FragColor = vec4(creatureColor, 1.0); // 色を使う
}