#pragma once
#include "../Primitive.h"

namespace gl {
    class Cylinder : public Primitive {
    public:
        Cylinder(int radial_subdivisions = 8, int height_subdivisions = 1);
        void makeShape() override;

    private:
        // Helper methods for indexed cylinder generation
        std::vector<unsigned int> createRingVertices(float y, float theta, int heightIndex, int totalHeightDivisions);
        void makeSegment(const std::vector<unsigned int>& bottomRing, const std::vector<unsigned int>& topRing);
        void makeCap(float y, const glm::vec3& normal, bool isTop);

        float radius = 0.5f;
        float height = 1.0f;
    };
}
