#pragma once
#include <iostream>
#include "../../engine/global/GDefines.hpp"

class Camera{
    private:
        struct MousePos {
            double x = 0.0;
            double y = 0.0;
        };

    public:
        Camera(Vector3 position, Vector3 target, Vector3 worldup);
        Camera(Vector3 position, float pitch, float yaw, Vector3 worldup);

        Vector3 position;
        Vector3 forward;
        Vector3 right;
        Vector3 up;
        Vector3 worldUp;

        float pitch;
        float yaw;
        float mouseSpeedX = 0.01f;
        float mouseSpeedY = 0.01f;
        float keyBoardSpeedX = 0.0f;
        float keyBoardSpeedY = 0.0f;
        float keyBoardSpeedZ = 0.0f;
        MousePos mouesePos;

    public:
        Matrix4 GetViewMatrix();
        Vector3 GetPosition() const { return position; }

        void MouserMovement(float x, float y);
        void UpdateCameraPosition();

    private:
        void UpdateCameraVectors();

};
