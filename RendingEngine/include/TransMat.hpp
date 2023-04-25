#pragma once
#include <iostream>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

class ModelMatTools{
    public:
        static glm::mat4 translate(float x, float y, float z);

        static glm::mat4 scale(float scale);

        static glm::mat4 rotateX(float angle);
        static glm::mat4 rotateY(float angle);
        static glm::mat4 rotateZ(float angle);
};

class ViewMatTools{
    public:
        static glm::mat4 view(glm::vec3 pos, float upDirect, float lookAt);
};
