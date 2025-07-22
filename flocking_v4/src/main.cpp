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

// openMP for parallel
#include <omp.h> // OpenMPのヘッダーを追加

// Custom headers
#include "Creature.h"
#include "Shader.h"

#include "Collider.h"

// --- グローバル変数 ---
// ウィンドウサイズ
int SCR_WIDTH = 1200;
int SCR_HEIGHT = 800;

// カメラ
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, -60.0f);
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

// 球形コライダーのリスト
std::vector<SphereCollider> colliders;

// VAO/VBO/EBO for Creature (円錐) - speciesIDごとに配列で管理
unsigned int creatureVAOs[3];
unsigned int creatureVBOs[3];
unsigned int creatureEBOs[3];
int creatureNumIndices[3]; // 円錐のインデックス数 (speciesIDごと)

// VAO/VBO for BoxHelper (境界線)
unsigned int boxVAO, boxVBO;
unsigned int boxFaceEBO;      // 塗りつぶし用EBOに名前を変更
unsigned int boxWireframeEBO; // ワイヤーフレーム用EBOを新しく追加
int boxNumIndices;
int boxWireframeNumIndices;

// 球形コライダーのVAO/VBO/EBO
unsigned int sphereVAO, sphereVBO, sphereEBO;
int sphereIndexCount = 0;
Shader *sphereShader = nullptr;

unsigned int planeVAO = 0, planeVBO = 0;

// シェーダープログラム
Shader *creatureShader = nullptr;
// Shader *boxShader = nullptr; // これを削除またはコメントアウト
Shader *transparentBoxShader = nullptr; // 新しいシェーダーを追加

// --- 関数プロトタイプ宣言 ---
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void updateCameraPosition(); // ★ New function prototype
void setupCreatureMesh(float radius, float height, int speciesID);
void setupBoxMesh();
void renderCreature(const Creature &creature);
void renderBox(float size);
void setupSphereMesh(float radius, int sectorCount, int stackCount);
void generateSphereMesh(std::vector<float> &vertices, std::vector<unsigned int> &indices, float radius, int sectorCount, int stackCount);
void setupPlane();
void drawPlane(Shader &shader, const glm::mat4 &view, const glm::mat4 &projection);

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

    glEnable(GL_DEPTH_TEST);                           // 深度テスト有効化
    glEnable(GL_MULTISAMPLE);                          // アンチエイリアシング有効化
    glEnable(GL_BLEND);                                // アルファブレンドを有効にする
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // 標準的なアルファブレンドの式

    // シェーダーのロードとコンパイル
    creatureShader = new Shader("bin/shaders/creature.vert", "bin/shaders/creature.frag");
    // boxShader = new Shader("bin/shaders/box.vert", "bin/shaders/box.frag"); // ワイヤーフレーム描画用 -> 削除
    transparentBoxShader = new Shader("bin/shaders/transparent_box.vert", "bin/shaders/transparent_box.frag"); // 新しいシェーダーをロード
                                                                                                               // sphere shader
    sphereShader = new Shader("bin/shaders/sphere.vert", "bin/shaders/sphere.frag");

    // 生物と境界ボックスのメッシュをセットアップ
    // speciesID=0 のCreature用 (radius: 0.2f, height: 0.5f)
    setupCreatureMesh(0.3f, 0.7f, 0);
    // speciesID=1 のCreature用 (radius: 0.3f, height: 0.7f)
    setupCreatureMesh(0.5f, 1.8f, 1);
    setupCreatureMesh(0.3f, 1.2f, 2);
    setupBoxMesh();

    // Creaturesの生成
    for (int i = 0; i < 450; ++i)
    {
        creatures.push_back(std::make_unique<Creature>(CUBE_SIZE, 0));
    }
    for (int i = 0; i < 30; ++i)
    {
        creatures.push_back(std::make_unique<Creature>(CUBE_SIZE, 1));
    }

    for (int i = 0; i < 50; ++i)
    {
        creatures.push_back(std::make_unique<Creature>(CUBE_SIZE, 2));
    }

    // Coliderの生成
    colliders.push_back(SphereCollider(glm::vec3(5.0f, -15.0f, 0.0f), 3.0f));
    colliders.push_back(SphereCollider(glm::vec3(-10.0f, -18.0f, 3.0f), 3.0f));

    setupSphereMesh(2.0f, 16, 16);

    Shader planeShader("bin/shaders/plane.vert", "bin/shaders/plane.frag");
    setupPlane();

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
        std::vector<Creature *> creaturePointers;
        for (const auto &c_ptr : creatures)
        {
            creaturePointers.push_back(c_ptr.get());
        }

#pragma omp parallel for // 並列化
        for (int i = 0; i < creatures.size(); ++i)
        {
            creatures[i]->update(creaturePointers, CUBE_SIZE, colliders);
        }

        // --- レンダリング ---
        creatureShader->use();
        creatureShader->setMat4("projection", projection);
        creatureShader->setMat4("view", view);

        for (const auto &c : creatures)
        {
            renderCreature(*c);
        }

        // 球形コライダーの描画
        sphereShader->use();
        sphereShader->setMat4("projection", projection);
        sphereShader->setMat4("view", view);

        for (const auto &collider : colliders)
        {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), collider.center);
            model = glm::scale(model, glm::vec3(collider.radius));
            sphereShader->setMat4("model", model);

            sphereShader->setVec3("color", glm::vec3(0.3f, 0.3f, 0.3f)); // 赤色など
            sphereShader->setFloat("alpha", 1.0f);                       // 少し透ける

            glBindVertexArray(sphereVAO);
            glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);
        }
        glBindVertexArray(0);

        drawPlane(planeShader, view, projection);

        // 半透明なBoxを後から描画
        transparentBoxShader->use();
        transparentBoxShader->setMat4("projection", projection); // ここで設定！
        transparentBoxShader->setMat4("view", view);             // ここで設定！
        renderBox(CUBE_SIZE);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    // リソースの解放
    glDeleteVertexArrays(2, creatureVAOs); // 配列で一括削除
    glDeleteBuffers(2, creatureVBOs);
    glDeleteBuffers(2, creatureEBOs);
    glDeleteVertexArrays(1, &boxVAO);
    glDeleteBuffers(1, &boxVBO);
    glDeleteBuffers(1, &boxFaceEBO);      // 面用EBOの解放
    glDeleteBuffers(1, &boxWireframeEBO); // 線画用EBOの解放 <-- 追加

    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteBuffers(1, &sphereVBO);
    glDeleteBuffers(1, &sphereEBO);

    delete creatureShader;
    delete transparentBoxShader;
    delete sphereShader;

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

// ★ Add this new function
void updateCameraPosition()
{
    // OrbitControlsの簡易代替
    float cameraX = cameraTarget.x + orbitRadius * cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    float cameraY = cameraTarget.y + orbitRadius * sin(glm::radians(pitch));
    float cameraZ = cameraTarget.z + orbitRadius * sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    cameraPos = glm::vec3(cameraX, cameraY, cameraZ);
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

    updateCameraPosition(); // ★ Call the new function here
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
}

// setupCreatureMesh 関数を引数にspeciesIDを追加
void setupCreatureMesh(float radius, float height, int speciesID)
{
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    generateConeData(vertices, indices, radius, height, 16);

    glGenVertexArrays(1, &creatureVAOs[speciesID]);
    glGenBuffers(1, &creatureVBOs[speciesID]);
    glGenBuffers(1, &creatureEBOs[speciesID]);

    glBindVertexArray(creatureVAOs[speciesID]);

    glBindBuffer(GL_ARRAY_BUFFER, creatureVBOs[speciesID]);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, creatureEBOs[speciesID]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    creatureNumIndices[speciesID] = indices.size(); // 各種別のインデックス数を保存
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
    switch (creature.speciesID)
    {
    case 0:
        color = glm::vec3(0.8f, 0.8f, 1.0f);
        break;
    case 1:
        color = glm::vec3(0.3f, 0.3f, 1.0f);
        break;
    case 2:
        color = glm::vec3(0.6f, 0.2f, 0.3f);
        break;
    // 必要に応じて追加
    default:
        color = glm::vec3(1.0f);
    }
    creatureShader->setVec3("creatureColor", color);

    // CreatureのspeciesIDに応じて適切なVAOとインデックス数をバインド
    glBindVertexArray(creatureVAOs[creature.speciesID]);
    glDrawElements(GL_TRIANGLES, creatureNumIndices[creature.speciesID], GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
void setupBoxMesh()
{
    // 立方体の頂点データ (変更なし)
    float boxVertices[] = {
        // Position
        -1.1f, -1.1f, 1.1f,
        1.1f, -1.1f, 1.1f,
        1.1f, 1.1f, 1.1f,
        -1.1f, 1.1f, 1.1f,
        -1.1f, -1.1f, -1.1f,
        1.1f, -1.1f, -1.1f,
        1.1f, 1.1f, -1.1f,
        -1.1f, 1.1f, -1.1f};

    // 立方体の面を描画するためのインデックスデータ
    unsigned int boxFaceIndices[] = {
        // Front face (0,1,2,3)
        0, 1, 2, 2, 3, 0,
        // Right face (1,5,6,2)
        1, 5, 6, 6, 2, 1,
        // Back face (7,6,5,4)
        7, 6, 5, 5, 4, 7,
        // Left face (4,0,3,7)
        4, 0, 3, 3, 7, 4,
        // Top face (3,2,6,7)
        3, 2, 6, 6, 7, 3,
        // Bottom face (4,5,1,0)
        4, 5, 1, 1, 0, 4};
    boxNumIndices = sizeof(boxFaceIndices) / sizeof(unsigned int);

    // 立方体のワイヤーフレームを描画するためのインデックスデータ
    unsigned int boxWireframeIndices[] = {
        // Front face lines
        0, 1, 1, 2, 2, 3, 3, 0,
        // Back face lines
        4, 5, 5, 6, 6, 7, 7, 4,
        // Connecting lines
        0, 4, 1, 5, 2, 6, 3, 7};
    boxWireframeNumIndices = sizeof(boxWireframeIndices) / sizeof(unsigned int);

    glGenVertexArrays(1, &boxVAO);
    glGenBuffers(1, &boxVBO);
    glGenBuffers(1, &boxFaceEBO);      // 面用EBOを生成
    glGenBuffers(1, &boxWireframeEBO); // 線画用EBOを生成

    glBindVertexArray(boxVAO);

    glBindBuffer(GL_ARRAY_BUFFER, boxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(boxVertices), boxVertices, GL_STATIC_DRAW);

    // 面用EBOのデータ設定
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, boxFaceEBO); // ここで boxFaceEBO にバインド
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(boxFaceIndices), boxFaceIndices, GL_STATIC_DRAW);

    // 線画用EBOのデータ設定
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, boxWireframeEBO); // ここで boxWireframeEBO にバインド
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(boxWireframeIndices), boxWireframeIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}
void renderBox(float size)
{
    // --- 1. 半透明な箱（面）を描画 ---
    glDepthMask(GL_FALSE); // 深度書き込みを無効化（半透明オブジェクトのため）

    transparentBoxShader->use();
    transparentBoxShader->setMat4("model", glm::scale(glm::mat4(1.0f), glm::vec3(size, size, size)));
    transparentBoxShader->setVec3("objectColor", glm::vec3(0.1f, 0.5f, 0.8f)); // 水槽の色 (水色系)
    transparentBoxShader->setFloat("alpha", 0.3f);                             // 透明度

    glBindVertexArray(boxVAO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, boxFaceEBO); // ★面用EBOをバインド
    glDrawElements(GL_TRIANGLES, boxNumIndices, GL_UNSIGNED_INT, 0);

    // --- 2. ワイヤーフレーム（線）を描画 ---
    glDepthMask(GL_TRUE); // 深度書き込みを有効に戻す（線は不透明なので）

    // 同じシェーダーを使う場合
    transparentBoxShader->setVec3("objectColor", glm::vec3(0.8f, 0.8f, 0.8f)); // ワイヤーフレームの色 (灰色)
    transparentBoxShader->setFloat("alpha", 1.0f);                             // ワイヤーフレームは不透明

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, boxWireframeEBO);               // ★線画用EBOをバインド
    glDrawElements(GL_LINES, boxWireframeNumIndices, GL_UNSIGNED_INT, 0); // GL_LINESで描画

    glBindVertexArray(0);
}

void setupSphereMesh(float radius = 1.0f, int sectorCount = 16, int stackCount = 16)
{
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    generateSphereMesh(vertices, indices, radius, sectorCount, stackCount);
    sphereIndexCount = (int)indices.size();

    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glGenBuffers(1, &sphereEBO);

    glBindVertexArray(sphereVAO);

    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void generateSphereMesh(std::vector<float> &vertices, std::vector<unsigned int> &indices, float radius, int sectorCount, int stackCount)
{
    vertices.clear();
    indices.clear();

    const float PI = glm::pi<float>();

    for (int i = 0; i <= stackCount; ++i)
    {
        float stackAngle = PI / 2 - i * PI / stackCount; // 上から下へ
        float xy = radius * cosf(stackAngle);
        float z = radius * sinf(stackAngle);

        for (int j = 0; j <= sectorCount; ++j)
        {
            float sectorAngle = j * 2 * PI / sectorCount;

            float x = xy * cosf(sectorAngle);
            float y = xy * sinf(sectorAngle);

            // 頂点位置のみ (x,y,z)
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
        }
    }

    // インデックス生成
    for (int i = 0; i < stackCount; ++i)
    {
        int k1 = i * (sectorCount + 1); // current stack
        int k2 = k1 + sectorCount + 1;  // next stack

        for (int j = 0; j < sectorCount; ++j, ++k1, ++k2)
        {
            if (i != 0)
            {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }

            if (i != (stackCount - 1))
            {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }
}

void setupPlane()
{
    float planeVertices[] = {
        // positions          // normals         // texcoords
        5.0f, 0.0f, 5.0f, 0.0f, 1.0f, 0.0f, 5.0f, 0.0f,
        -5.0f, 0.0f, 5.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
        -5.0f, 0.0f, -5.0f, 0.0f, 1.0f, 0.0f, 0.0f, 5.0f,

        5.0f, 0.0f, 5.0f, 0.0f, 1.0f, 0.0f, 5.0f, 0.0f,
        -5.0f, 0.0f, -5.0f, 0.0f, 1.0f, 0.0f, 0.0f, 5.0f,
        5.0f, 0.0f, -5.0f, 0.0f, 1.0f, 0.0f, 5.0f, 5.0f};

    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);

    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);

    // positions
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // normals
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // texcoords
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void drawPlane(Shader &shader, const glm::mat4 &view, const glm::mat4 &projection)
{
    shader.use();
    glm::mat4 model = glm::mat4(1.0f);

    model = glm::translate(model, glm::vec3(0.0f, -22.1f, 0.0f)); // Y軸方向に下げる
    model = glm::scale(model, glm::vec3(10.0f, 1.0f, 10.0f));     // XZ方向に広げる

    shader.setMat4("model", model);
    shader.setMat4("view", view);
    shader.setMat4("projection", projection);

    glBindVertexArray(planeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}