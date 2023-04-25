#include "Camera.hpp"

Camera::Camera(glm::vec3 position, glm::vec3 target, glm::vec3 worldup){
    this->position = position;
    this->worldUp = worldup;
    this->forward = glm::normalize(target-position);
    this->right = glm::normalize(glm::cross(forward, worldup));
    this->up = glm::normalize(glm::cross(right, forward));
}

Camera::Camera(glm::vec3 position, float pitch, float yaw, glm::vec3 worldup){
    this->position = position;
    this->pitch = pitch;
    this->yaw = yaw;
    this->worldUp = worldup;
    this->forward.x = glm::cos(pitch) * glm::sin(yaw);
    this->forward.y = glm::sin(pitch);
    this->forward.z = glm::cos(pitch) * glm::cos(yaw);
    this->right = glm::normalize(glm::cross(forward, worldup));
    this->up = glm::normalize(glm::cross(right, forward));
}

glm::mat4 Camera::getViewMatrix(){
    return glm::lookAt(position,position + forward, worldUp);
}

void Camera::mouserMovement(float x, float y){  
    pitch += y * mouseSpeedX;
    yaw += x * mouseSpeedY;
    if(pitch >  89.0f) pitch =  89.0f;
    if(pitch < -89.0f) pitch = -89.0f;
    if(yaw > 179.0f) yaw =  -179.0f;
    if(yaw < -179.0f) yaw = 179.0f;

    updateCameraVectors();
}

void Camera::updateCameraVectors(){
    this->forward.x = glm::cos(pitch) * glm::sin(yaw);
    this->forward.y = glm::sin(pitch);
    this->forward.z = glm::cos(pitch) * glm::cos(yaw);
    this->right = glm::normalize(glm::cross(forward, worldUp));
    this->up = glm::normalize(glm::cross(right, forward));
}

void Camera::updateCameraPosition(){
    position += forward * keyBoardSpeedZ * 0.01f 
             + right * keyBoardSpeedX * 0.01f
             + up * keyBoardSpeedY * 0.01f;
}