#include "Cube.h"


namespace gl {
    Cube::Cube(int subdivisions) {
        param_1 = std::max(1, subdivisions);
        param_2 = std::max(1, subdivisions);
        Cube::makeShape();
    }

    void Cube::makeShape() {
        float size = 1.f;
        const float halfSize = size / 2.0f;

        // Standard cube unwrap UV layout (cross pattern):
        // Texture is divided into 3x4 grid, with faces arranged as:
        //       [Top]
        // [L]  [Front]  [R]  [Back]
        //       [Bot]
        // Each face occupies 1/4 width × 1/3 height

        const float uStep = 1.0f / 4.0f;  // Width of one face
        const float vStep = 1.0f / 3.0f;  // Height of one face

        // Front face (+Z) - Middle center
        makeFace(glm::vec3(0, 0, 1), glm::vec3(1, 0, 0), glm::vec3(0, 1, 0),
                 glm::vec3(0, 0, halfSize), size,
                 glm::vec2(uStep, vStep), glm::vec2(2.0f * uStep, 2.0f * vStep));

        // Back face (-Z) - Right center
        makeFace(glm::vec3(0, 0, -1), glm::vec3(-1, 0, 0), glm::vec3(0, 1, 0),
                 glm::vec3(0, 0, -halfSize), size,
                 glm::vec2(3.0f * uStep, vStep), glm::vec2(4.0f * uStep, 2.0f * vStep));

        // Right face (+X) - Middle right
        makeFace(glm::vec3(1, 0, 0), glm::vec3(0, 0, -1), glm::vec3(0, 1, 0),
                 glm::vec3(halfSize, 0, 0), size,
                 glm::vec2(2.0f * uStep, vStep), glm::vec2(3.0f * uStep, 2.0f * vStep));

        // Left face (-X) - Middle left
        makeFace(glm::vec3(-1, 0, 0), glm::vec3(0, 0, 1), glm::vec3(0, 1, 0),
                 glm::vec3(-halfSize, 0, 0), size,
                 glm::vec2(0.0f, vStep), glm::vec2(uStep, 2.0f * vStep));

        // Top face (+Y) - Top center
        makeFace(glm::vec3(0, 1, 0), glm::vec3(1, 0, 0), glm::vec3(0, 0, -1),
                 glm::vec3(0, halfSize, 0), size,
                 glm::vec2(uStep, 2.0f * vStep), glm::vec2(2.0f * uStep, 3.0f * vStep));

        // Bottom face (-Y) - Bottom center
        makeFace(glm::vec3(0, -1, 0), glm::vec3(1, 0, 0), glm::vec3(0, 0, 1),
                 glm::vec3(0, -halfSize, 0), size,
                 glm::vec2(uStep, 0.0f), glm::vec2(2.0f * uStep, vStep));
    }

    void Cube::makeFace(const glm::vec3& normal, const glm::vec3& tangent, const glm::vec3& bitangent,
                        const glm::vec3& center, float faceSize,
                        const glm::vec2& uvMin, const glm::vec2& uvMax) {
        const int subdivisions = param_1;
        const float halfSize = faceSize / 2.0f;
        const float step = faceSize / float(subdivisions);

        // Create a grid of vertices for this face
        std::vector<std::vector<unsigned int>> vertexGrid(subdivisions + 1);

        for (int i = 0; i <= subdivisions; i++) {
            vertexGrid[i].reserve(subdivisions + 1);
            for (int j = 0; j <= subdivisions; j++) {
                // Calculate position on the face
                const float u = -halfSize + float(i) * step;
                const float v = -halfSize + float(j) * step;

                const glm::vec3 pos = center + u * tangent + v * bitangent;

                // Texture coordinates mapped to the face's UV region
                const float texU = uvMin.x + (float(i) / float(subdivisions)) * (uvMax.x - uvMin.x);
                const float texV = uvMin.y + (float(j) / float(subdivisions)) * (uvMax.y - uvMin.y);
                const glm::vec2 texcoord(texU, texV);


                // Insert vertex and store index
                unsigned int idx = insertVertex(pos, normal, texcoord);
                vertexGrid[i].push_back(idx);
            }
        }

        // Build triangles for this face (two triangles per quad)
        for (int i = 0; i < subdivisions; i++) {
            for (int j = 0; j < subdivisions; j++) {
                // Get the four corners of the quad
                unsigned int bottomLeft = vertexGrid[i][j];
                unsigned int bottomRight = vertexGrid[i + 1][j];
                unsigned int topLeft = vertexGrid[i][j + 1];
                unsigned int topRight = vertexGrid[i + 1][j + 1];

                // Triangle 1: bottom-left, bottom-right, top-right
                insertIndex(bottomLeft);
                insertIndex(bottomRight);
                insertIndex(topRight);

                // Triangle 2: bottom-left, top-right, top-left
                insertIndex(bottomLeft);
                insertIndex(topRight);
                insertIndex(topLeft);
            }
        }
    }
}

