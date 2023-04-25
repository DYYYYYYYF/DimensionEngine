#pragma once
#include <iostream>
class Loader{
    public:
        Loader(){}
        ~Loader(){}
        template<typename T, typename D>
        void loadCube(T &vertices, D &indices);
        void loadeModel();
};