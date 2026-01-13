#pragma once
#include "glm/glm.hpp"


namespace gl {
    struct DrawShape;

    class Primitive {
    public:
        virtual ~Primitive() = default;
        void updateParams(int param1, int param2);

        DrawShape loadDrawShape() const;

        std::vector<float> getBufferData() const;
        std::vector<unsigned int> getIndices() const;
        std::vector<glm::vec3> getVertexData() const;

    protected:
        void insertBufferData(glm::vec3 pos, glm::vec3 normal, glm::vec2 tex);
        unsigned int insertVertex(glm::vec3 pos, glm::vec3 normal, glm::vec2 tex);
        void insertIndex(unsigned int index);
        virtual void makeShape() = 0;
        int param_1;
        int param_2;
        std::vector<float> buffer_data_;
        std::vector<glm::vec3> vertices_;
        std::vector<unsigned int> indices_;
};


}
