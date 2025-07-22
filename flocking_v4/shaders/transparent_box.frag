#version 330 core
out vec4 FragColor;

uniform vec3 objectColor;
uniform float alpha; // 新しく追加するアルファ値

void main()
{
    // 受け取った色とアルファ値を組み合わせる
    FragColor = vec4(objectColor, alpha);
}