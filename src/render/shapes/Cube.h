#pragma once
#include "../Primitive.h"

namespace gl {
    class Cube : public Primitive {
    public:
        Cube(int subdivisions = 1);
        void makeShape() override;

    private:
        void makeFace(const glm::vec3& normal, const glm::vec3& tangent, const glm::vec3& bitangent,
                      const glm::vec3& center, float size,
                      const glm::vec2& uvMin, const glm::vec2& uvMax);
    };
}
