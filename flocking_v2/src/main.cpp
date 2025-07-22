#define GLM_ENABLE_EXPERIMENTAL // Add this line

#include <iostream>
#include <vector>
#include <memory>

// OpenGL and GLFW
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// GLM for mathematics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp> // This is the header that triggered the error

// Custom headers
#include "Creature.h"
#include "Shader.h"

// --- グローバル変数 ---
// ウィンドウサイズ
int SCR_WIDTH = 800;
int SCR_HEIGHT = 600;

// カメラ
glm::vec3 cameraPos = glm::vec3(30.0f, 30.0f, 30.0f);
glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

// OrbitControlsの簡易的な代替 (マウス入力用)
float yaw = -90.0f; // Y軸周りの回転 (横方向)
float pitch = 0.0f; // X軸周りの回転 (縦方向)
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
float cameraSpeed = 0.1f; // カメラの移動速度
float orbitRadius = glm::distance(cameraPos, cameraTarget);

// 生物のリスト
std::vector<std::unique_ptr<Creature>> creatures;
const float CUBE_SIZE = 20.0f;

// VAO/VBO/EBO for Creature (円錐)
unsigned int creatureVAO, creatureVBO, creatureEBO;
int creatureNumIndices; // 円錐のインデックス数

// VAO/VBO for BoxHelper (境界線)
unsigned int boxVAO, boxVBO, boxEBO; // boxEBO を追加
int boxNumVertices;

// シェーダープログラム
Shader *creatureShader = nullptr;
Shader *boxShader = nullptr;

// --- 関数プロトタイプ宣言 ---
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void setupCreatureMesh();
void setupBoxMesh();
void renderCreature(const Creature &creature);
void renderBox(float size);

int main()
{
    // GLFW初期化
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // OpenGLバージョンとプロファイル設定
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // macOS用
#endif
    glfwWindowHint(GLFW_SAMPLES, 4); // アンチエイリアシング

    // ウィンドウ作成
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Flocking Creatures C++", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // コールバック関数登録
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // カーソルを隠して、入力として使う
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Glad初期化
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);  // 深度テスト有効化
    glEnable(GL_MULTISAMPLE); // アンチエイリアシング有効化

    // シェーダーのロードとコンパイル
    creatureShader = new Shader("bin/shaders/creature.vert", "bin/shaders/creature.frag");
    boxShader = new Shader("bin/shaders/box.vert", "bin/shaders/box.frag"); // ワイヤーフレーム描画用

    // 生物と境界ボックスのメッシュをセットアップ
    setupCreatureMesh();
    setupBoxMesh();

    // Creaturesの生成
    for (int i = 0; i < 200; ++i)
    {
        creatures.push_back(std::make_unique<Creature>(CUBE_SIZE, 0));
    }
    for (int i = 0; i < 100; ++i)
    {
        creatures.push_back(std::make_unique<Creature>(CUBE_SIZE, 1)); 
    }

    // メインループ
    while (!glfwWindowShouldClose(window))
    {
        // 入力処理
        processInput(window);

        // 背景色
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // --- View/Projection行列の計算 ---
        glm::mat4 projection = glm::perspective(glm::radians(75.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, cameraUp);

        // Creatureの更新
        // shared_ptrやreference_wrapperを使うとより洗練されますが、
        // 簡易的にポインタリストを生成
        std::vector<Creature *> creaturePointers;

        for (const auto &c_ptr : creatures)
        {
            creaturePointers.push_back(c_ptr.get());
        }
        for (auto &c : creatures)
        {
            c->update(creaturePointers, CUBE_SIZE);
        }

        // --- レンダリング ---
        creatureShader->use();
        creatureShader->setMat4("projection", projection);
        creatureShader->setMat4("view", view);

        for (const auto &c : creatures)
        {
            renderCreature(*c);
        }

        boxShader->use();
        boxShader->setMat4("projection", projection);
        boxShader->setMat4("view", view);
        renderBox(CUBE_SIZE);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // リソースの解放
    glDeleteVertexArrays(1, &creatureVAO);
    glDeleteBuffers(1, &creatureVBO);
    glDeleteBuffers(1, &creatureEBO);
    glDeleteVertexArrays(1, &boxVAO);
    glDeleteBuffers(1, &boxVBO);
    glDeleteBuffers(1, &boxEBO); // boxEBO の解放を追加

    delete creatureShader;
    delete boxShader;

    glfwTerminate();
    return 0;
}

// --- コールバック関数と入力処理 ---
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
}

void mouse_callback(GLFWwindow *window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Yを反転
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch -= yoffset;

    // ピッチの制限
    if (pitch > 30.0f)
        pitch = 30.0f;
    if (pitch < -30.0f)
        pitch = -30.0f;

    // OrbitControlsの簡易代替
    float cameraX = cameraTarget.x + orbitRadius * cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    float cameraY = cameraTarget.y + orbitRadius * sin(glm::radians(pitch));
    float cameraZ = cameraTarget.z + orbitRadius * sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    cameraPos = glm::vec3(cameraX, cameraY, cameraZ);
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    orbitRadius -= static_cast<float>(yoffset);
    if (orbitRadius < 1.0f)
        orbitRadius = 1.0f; // 最小ズーム
    if (orbitRadius > 100.0f)
        orbitRadius = 100.0f; // 最大ズーム
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// --- メッシュのセットアップとレンダリング関数 ---

// 円錐の頂点データを生成するヘルパー関数
// Three.jsのConeGeometry(0.2, 0.5, 16)に相当
void generateConeData(std::vector<float> &vertices, std::vector<unsigned int> &indices, float radius, float height, int radialSegments)
{
    vertices.clear();
    indices.clear();

    // 頂点データ: X, Y, Z (position)
    // 底面の中心
    vertices.push_back(0.0f);
    vertices.push_back(-height / 2.0f);
    vertices.push_back(0.0f);

    // 底面の円周
    for (int i = 0; i < radialSegments; ++i)
    {
        float angle = static_cast<float>(i) / radialSegments * glm::pi<float>() * 2.0f;
        vertices.push_back(radius * cos(angle));
        vertices.push_back(-height / 2.0f);
        vertices.push_back(radius * sin(angle));
    }

    // 頂点 (apex)
    vertices.push_back(0.0f);
    vertices.push_back(height / 2.0f);
    vertices.push_back(0.0f);

    // インデックスデータ
    // 底面 (扇形)
    for (int i = 0; i < radialSegments; ++i)
    {
        indices.push_back(0); // 中心
        indices.push_back(i + 1);
        indices.push_back((i + 1) % radialSegments + 1);
    }

    // 側面 (三角形の帯)
    unsigned int apexIndex = radialSegments + 1;
    for (int i = 0; i < radialSegments; ++i)
    {
        indices.push_back(apexIndex); // 頂点
        indices.push_back((i + 1) % radialSegments + 1);
        indices.push_back(i + 1);
    }

    creatureNumIndices = indices.size();
}

void setupCreatureMesh()
{
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    generateConeData(vertices, indices, 0.2f, 0.5f, 16); // Three.jsのパラメータ

    glGenVertexArrays(1, &creatureVAO);
    glGenBuffers(1, &creatureVBO);
    glGenBuffers(1, &creatureEBO);

    glBindVertexArray(creatureVAO);

    glBindBuffer(GL_ARRAY_BUFFER, creatureVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, creatureEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void renderCreature(const Creature &creature)
{
    // Model行列の計算
    // Three.jsの quat.setFromUnitVectors(new THREE.Vector3(0, -1, 0), this.direction);
    // に相当するGLMでの回転
    // Three.jsのConeGeometryはY軸上向き (0,1,0) に生成され、Creatureクラスでは (-Y, 0,-1,0) が進む方向。
    // そのため、GLMでは、モデルがデフォルトで向いている方向 (Three.jsのコーンの軸方向、例えばY軸プラス方向) から
    // Creatureの `direction` へ回転させるクォータニオンを計算します。
    // Creatureの速度が負の値だったので、その向きを反転させて (-direction) を使うことで、
    // 円錐の底面が進む方向に向くようにします。
    glm::quat orientation = glm::rotation(glm::vec3(0.0f, 1.0f, 0.0f), glm::normalize(-creature.direction));

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, creature.position);
    model = model * glm::toMat4(orientation); // クォータニオンから行列に変換

    creatureShader->setMat4("model", model);



    glm::vec3 color;
    switch (creature.speciesID) {
        case 0: color = glm::vec3(1.0f, 0.2f, 0.2f); break;
        case 1: color = glm::vec3(0.2f, 0.2f, 1.0f); break;
        // 必要に応じて追加
        default: color = glm::vec3(1.0f);
    }
    creatureShader->setVec3("creatureColor", color);


    glBindVertexArray(creatureVAO);
    glDrawElements(GL_TRIANGLES, creatureNumIndices, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
void setupBoxMesh()
{
    // 12本の線で構成される立方体の頂点
    float boxVertices[] = {
        // Front face
        -1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        // Back face
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f};

    unsigned int boxIndices[] = {
        // Front face lines
        0, 1, // 0-1
        1, 2, // 1-2
        2, 3, // 2-3
        3, 0, // 3-0
        // Back face lines
        4, 5, // 4-5
        5, 6, // 5-6
        6, 7, // 6-7
        7, 4, // 7-4
        // Connecting lines
        0, 4, // 0-4
        1, 5, // 1-5
        2, 6, // 2-6
        3, 7  // 3-7
    };
    // boxNumVertices は boxIndices の要素数にする
    // glDrawElementsを使うので、頂点数ではなくインデックス数が必要
    boxNumVertices = sizeof(boxIndices) / sizeof(unsigned int); // 名前はboxNumIndicesの方が適切かもしれません

    glGenVertexArrays(1, &boxVAO);
    glGenBuffers(1, &boxVBO);
    unsigned int boxEBO;      // EBOを追加
    glGenBuffers(1, &boxEBO); // EBOを生成

    glBindVertexArray(boxVAO);

    glBindBuffer(GL_ARRAY_BUFFER, boxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(boxVertices), boxVertices, GL_STATIC_DRAW);

    // EBOをバインドし、データを送る
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, boxEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(boxIndices), boxIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    // EBOもmain関数の終了時に削除できるように、グローバル変数にするか、ここでDeleteBuffersを呼ぶ
    // 今回はmainでまとめて削除できるように、boxEBOもグローバル変数として宣言することを推奨
    // main.cppのグローバル変数に `unsigned int boxEBO;` を追加してください
}

void renderBox(float size)
{
    boxShader->setMat4("model", glm::scale(glm::mat4(1.0f), glm::vec3(size, size, size))); // スケールを適用
    boxShader->setVec3("lineColor", glm::vec3(0.53f, 0.53f, 0.53f));                       // 0x888888

    glBindVertexArray(boxVAO);
    // glDrawArrays の代わりに glDrawElements を使用
    glDrawElements(GL_LINES, boxNumVertices, GL_UNSIGNED_INT, 0); // GL_LINESで描画
    glBindVertexArray(0);
}