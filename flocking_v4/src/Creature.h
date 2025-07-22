#ifndef CREATURE_H
#define CREATURE_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp> // For glm::quat
#include <vector>

#include "Collider.h"

class Creature
{
public:
    int speciesID; // 群れの種類

    glm::vec3 position;
    glm::vec3 direction; // 進行方向
    float speed;
    float maxTurn;

    Creature(float cubeSize, int speciesID);
void update(std::vector<Creature *> &others, float cubeSize, const std::vector<SphereCollider> &colliders);

    private : void flock(std::vector<Creature *> &others);
    void reflect(const glm::vec3 &normal);
};

#endif