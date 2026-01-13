#pragma once
#include "../Primitive.h"

namespace gl {
    class Cone : public Primitive {
    public:
        Cone(int radial_subdivisions = 8, int height_subdivisions = 4);
        void makeShape() override;

    private:
        // Helper methods for indexed cone generation
        std::vector<unsigned int> createWedgeVertices(float theta, int numHeightDivisions);
        void makeWedge(const std::vector<unsigned int>& currIndices,
                       const std::vector<unsigned int>& nextIndices);
        void makeBaseCap(float thetaStep, int numWedges);

        float radius = 0.5f;
        float height = 1.0f;
    };
}
