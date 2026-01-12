#pragma once
#include "../Primitive.h"

namespace gl {
    class Cone : public Primitive {
    public:
        Cone(int quality = 8);
        void makeShape() override;
    private:
        void makeWedge(float curr_theta, float next_theta);
        float radius = 0.5f;
        float height = 1.0f;
    };
}
