#define GLM_ENABLE_EXPERIMENTAL // Add this line

#include "Creature.h"
#include <glm/gtx/rotate_vector.hpp> // For glm::reflect
#include <glm/gtx/norm.hpp> // For glm::length2
#include <glm/gtx/transform.hpp> // For glm::mix
#include <random> // より良い乱数生成

// 乱数生成器
static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_real_distribution<> dis(-1.0, 1.0); // -1.0から1.0の間の乱数

Creature::Creature(float cubeSize) {
    position.x = dis(gen) * cubeSize;
    position.y = dis(gen) * cubeSize;
    position.z = dis(gen) * cubeSize;

    // 球面上のランダムな方向を生成
    float theta = dis(gen) * glm::pi<float>(); // 0から2PI
    float phi = std::acos(dis(gen)); // 0からPI (acosの引数を-1から1にすることで均一な分布)

    direction = glm::vec3(
        std::sin(phi) * std::cos(theta),
        std::cos(phi),
        std::sin(phi) * std::sin(theta)
    );
    direction = glm::normalize(direction);

    speed = 0.03f; // JavaScriptコードに合わせる
    maxTurn = 0.1f;
}

void Creature::update(std::vector<Creature*>& others, float cubeSize) {
    flock(others);

    glm::vec3 moveVec = direction * speed;
    position += moveVec;

    // 境界チェックと反射
    if (position.x > cubeSize) { position.x = cubeSize; reflect(glm::vec3(-1, 0, 0)); }
    else if (position.x < -cubeSize) { position.x = -cubeSize; reflect(glm::vec3(1, 0, 0)); }

    if (position.y > cubeSize) { position.y = cubeSize; reflect(glm::vec3(0, -1, 0)); }
    else if (position.y < -cubeSize) { position.y = -cubeSize; reflect(glm::vec3(0, 1, 0)); }

    if (position.z > cubeSize) { position.z = cubeSize; reflect(glm::vec3(0, 0, -1)); }
    else if (position.z < -cubeSize) { position.z = -cubeSize; reflect(glm::vec3(0, 0, 1)); }
}

void Creature::flock(std::vector<Creature*>& others) {
    glm::vec3 separation(0.0f);
    glm::vec3 alignment(0.0f);
    glm::vec3 cohesion(0.0f);
    int count = 0;

    float radius = 5.0f;
    float radiusSq = radius * radius; // 距離の二乗で比較して平方根の計算を避ける

    for (Creature* other : others) {
        if (other == this) continue;
        glm::vec3 diffVec = position - other->position;
        float dSq = glm::dot(diffVec, diffVec); // 距離の二乗

        if (dSq < radiusSq && dSq > 0.0001f) { // dSqが0に近い場合を除外
            separation += glm::normalize(diffVec) / (dSq + 0.01f); // d*d + 0.01f を dSq + 0.01f に変更

            alignment += other->direction;
            cohesion += other->position;
            count++;
        }
    }

    if (count > 0) {
        separation /= static_cast<float>(count);
        alignment = glm::normalize(alignment / static_cast<float>(count));
        cohesion = glm::normalize(cohesion / static_cast<float>(count) - position);

        glm::vec3 steer = glm::vec3(0.0f);
        steer += separation * 250.0f;
        steer += alignment * 3.0f;
        steer += cohesion * 10.0f;
        steer = glm::normalize(steer);

        // lerp (線形補間) を使用し、結果を正規化
        direction = glm::normalize(glm::mix(direction, steer, maxTurn));
    }
}

void Creature::reflect(const glm::vec3& normal) {
    direction = glm::reflect(direction, normal);
    direction = glm::normalize(direction);
}