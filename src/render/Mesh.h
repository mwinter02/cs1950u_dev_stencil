#pragma once
#include "Graphics.h"


struct aiMaterial;
struct aiScene;
struct aiTexture;

namespace gl {
    struct DrawShape;





    class Mesh {
    public:


        static DrawShape loadStaticShape(const std::vector<float> &data);
        static DrawMesh loadStaticMesh(const char* filename);

    private:


    };
}
