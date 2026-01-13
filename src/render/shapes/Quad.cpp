#include "Quad.h"

namespace gl {
    Quad::Quad(int subdivisions) {
        param_1 = std::max(1, subdivisions);
        param_2 = std::max(1, subdivisions);
        Quad::makeShape();
    }

    void Quad::makeShape() {
        const int subdivisions = param_1;
        const float halfSize = size / 2.0f;
        const float step = size / float(subdivisions);

        // Normal pointing up (+Y)
        const glm::vec3 normal(0.0f, 1.0f, 0.0f);

        // Create a grid of vertices on the XZ plane (Y = 0)
        std::vector<std::vector<unsigned int>> vertexGrid(subdivisions + 1);

        for (int i = 0; i <= subdivisions; i++) {
            vertexGrid[i].reserve(subdivisions + 1);
            for (int j = 0; j <= subdivisions; j++) {
                // Position on XZ plane, centered at origin
                const float x = -halfSize + float(i) * step;
                const float z = -halfSize + float(j) * step;
                const glm::vec3 position(x, 0.0f, z);

                // UV coordinates (0 to 1)
                const float u = float(i) / float(subdivisions);
                const float v = float(j) / float(subdivisions);
                const glm::vec2 texcoord(u, v);

                unsigned int idx = insertVertex(position, normal, texcoord);
                vertexGrid[i].push_back(idx);
            }
        }

        // Build triangles (two triangles per quad)
        for (int i = 0; i < subdivisions; i++) {
            for (int j = 0; j < subdivisions; j++) {
                // Get the four corners of the quad
                unsigned int bottomLeft = vertexGrid[i][j];
                unsigned int bottomRight = vertexGrid[i + 1][j];
                unsigned int topLeft = vertexGrid[i][j + 1];
                unsigned int topRight = vertexGrid[i + 1][j + 1];

                // Triangle 1: bottom-left, top-right, bottom-right (counter-clockwise from above)
                insertIndex(bottomLeft);
                insertIndex(topRight);
                insertIndex(bottomRight);

                // Triangle 2: bottom-left, top-left, top-right (counter-clockwise from above)
                insertIndex(bottomLeft);
                insertIndex(topLeft);
                insertIndex(topRight);
            }
        }
    }
}

