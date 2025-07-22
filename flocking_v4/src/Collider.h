// Collider.h
#pragma once
#include <glm/glm.hpp>

class SphereCollider
{
public:
    glm::vec3 center;
    float radius;

    SphereCollider(const glm::vec3 &c, float r) : center(c), radius(r) {}

    // Creatureの「点」が球の内側にあるかどうかを判定します
    bool isColliding(const glm::vec3 &pos) const
    {
        // 厳密にめり込んでいるかを判定するため、マージンは0で考えます
        return glm::distance(pos, center) < radius;
    }
};