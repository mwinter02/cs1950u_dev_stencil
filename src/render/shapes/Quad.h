#pragma once
#include "../Primitive.h"

namespace gl {
    class Quad : public Primitive {
    public:
        Quad(int subdivisions = 1);
        void makeShape() override;

    private:
        float size = 1.0f;
    };
}
