#pragma once
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc//matrix_transform.hpp>

namespace engine {
class Camera{
    private:
        struct MousePos {
            double x = 0.0;
            double y = 0.0;
        };

    public:
        Camera(glm::vec3 position, glm::vec3 target, glm::vec3 worldup);
        Camera(glm::vec3 position, float pitch, float yaw, glm::vec3 worldup);

        glm::vec3 position;
        glm::vec3 forward;
        glm::vec3 right;
        glm::vec3 up;
        glm::vec3 worldUp;

        float pitch;
        float yaw;
        float mouseSpeedX = 0.01f;
        float mouseSpeedY = 0.01f;
        float keyBoardSpeedX = 0.0f;
        float keyBoardSpeedY = 0.0f;
        float keyBoardSpeedZ = 0.0f;
        MousePos mouesePos;

    public:
        glm::mat4 GetViewMatrix();

        void MouserMovement(float x, float y);
        void UpdateCameraPosition();

    private:
        void UpdateCameraVectors();

};
}
