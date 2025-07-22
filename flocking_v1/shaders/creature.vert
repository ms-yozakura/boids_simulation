#version 410 core // これになっているか確認

// 以前の in vec3 aPos; ではなく、以下のように変更
layout (location = 0) in vec3 aPos; 

// 他の入力変数がある場合も同様に location を指定する
// layout (location = 1) in vec3 aNormal;
// layout (location = 2) in vec2 aTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}