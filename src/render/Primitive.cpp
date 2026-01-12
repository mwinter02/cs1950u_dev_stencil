#include "Primitive.h"

namespace gl {
    static void insertVec3(std::vector<float>& data, const glm::vec3 v) {
        data.push_back(v.x);
        data.push_back(v.y);
        data.push_back(v.z);
    }

    static void insertVec2(std::vector<float>& data, const glm::vec2 v) {
        data.push_back(v.x);
        data.push_back(v.y);
    }

    void Primitive::insertBufferData(glm::vec3 pos, glm::vec3 normal, glm::vec2 tex) {
        insertVec3(buffer_data_, pos);
        insertVec3(buffer_data_, normal);
        insertVec2(buffer_data_, tex);
        vertices_.push_back(pos);
    }

    void Primitive::updateParams(const int param1, const int param2) {
        buffer_data_.clear();
        vertices_.clear();

        param_1 = param1;
        param_2 = param2;
        makeShape();
    }

    std::vector<float> Primitive::getBufferData() const {
        return buffer_data_;
    }

    std::vector<glm::vec3> Primitive::getVertexData() const {
        return vertices_;
    }
}

