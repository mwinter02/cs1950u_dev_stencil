#pragma once
#include "Graphics.h"


struct aiMaterial;
struct aiScene;
struct aiTexture;

namespace gl {
    class Primitive;
    struct DrawShape;

    class Mesh {
    public:

        static DrawShape loadStaticShapeIndexed(const std::vector<float>& buffer_data, const std::vector<unsigned int>& indices);
        static DrawShape loadPrimitive(const Primitive& primitive);
        static DrawMesh loadStaticMesh(const char* filename);

    private:


    };
}
