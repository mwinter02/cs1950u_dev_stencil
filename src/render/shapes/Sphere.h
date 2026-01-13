#pragma once
#include "../Primitive.h"

namespace gl {
    class Sphere : public Primitive {
    public:
        Sphere(int latitudinal_subdivisions = 16, int longitudinal_subdivisions = 32);
        void makeShape() override;

    private:
        float radius = 0.5f;
    };
}
