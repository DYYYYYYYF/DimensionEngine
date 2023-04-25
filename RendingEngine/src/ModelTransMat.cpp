#include "TransMat.hpp"

/*
    模型平移矩阵
*/
glm::mat4 ModelMatTools::translate(float x, float y, float z){
    glm::mat4 trans = glm::mat4(1, 0, 0, 0,
                                0, 1, 0, 0,
                                0, 0, 1, 0,
                                -x, -y, -z, 1);
    return trans;
}

/*
    模型缩放矩阵
*/
glm::mat4 ModelMatTools::scale(float scale){
    glm::mat4 trans = glm::mat4(scale, 0, 0, 0,
                                0, scale, 0, 0,
                                0, 0, scale, 0,
                                0,  0,   0,  1);
    return trans;
}


/*
    模型旋转矩阵
*/
glm::mat4 ModelMatTools::rotateX(float angle){
    glm::mat4 trans = glm::mat4(1, 0,               0,                0,
                                0, std::cos(angle), -std::sin(angle), 0,
                                0, std::sin(angle),  std::cos(angle), 0,
                                0, 0,                0,               1);
    return trans;
}

glm::mat4 ModelMatTools::rotateY(float angle){
    glm::mat4 trans = glm::mat4( std::cos(angle), 0, std::sin(angle), 0,
                                 0,               1, 0,               0,
                                -std::sin(angle), 0, std::cos(angle), 0,
                                 0,               0, 0,               1);
    return trans;
}

glm::mat4 ModelMatTools::rotateZ(float angle){
    glm::mat4 trans = glm::mat4(std::cos(angle), -std::sin(angle), 0, 0,
                                std::sin(angle),  std::cos(angle), 0, 0,
                                0,                0,               1, 0,
                                0,                0,               0, 1);
    return trans;
}