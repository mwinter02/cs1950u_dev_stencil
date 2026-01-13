#include "Cone.h"

namespace gl {
    Cone::Cone(int radial_subdivisions, int height_subdivisions) {
        param_1 = height_subdivisions;  // Height subdivisions (at least 4)
        param_2 = radial_subdivisions;                    // Radial subdivisions
        Cone::makeShape();
    }

    void Cone::makeShape() {
        const int numWedges = std::max(3, param_2);
        const int numHeightDivisions = std::max(1, param_1);
        const float thetaStep = glm::radians(360.f / float(numWedges));

        // Create vertex grid: (numWedges + 1) columns to allow wrapping
        // vertexGrid[wedge][height] = vertex index
        std::vector<std::vector<unsigned int>> vertexGrid(numWedges + 1);

        // Generate vertices for each wedge edge (last column wraps to first)
        for (int wedge = 0; wedge <= numWedges; wedge++) {
            if (wedge < numWedges) {
                const float theta = float(wedge) * thetaStep;
                vertexGrid[wedge] = createWedgeVertices(theta, numHeightDivisions);
            } else {
                // Last column shares vertices with first column for seamless wrapping
                vertexGrid[wedge] = vertexGrid[0];
            }
        }

        // Build triangles for cone surface
        for (int wedge = 0; wedge < numWedges; wedge++) {
            makeWedge(vertexGrid[wedge], vertexGrid[wedge + 1]);
        }

        // Build base cap
        makeBaseCap(thetaStep, numWedges);
    }

    std::vector<unsigned int> Cone::createWedgeVertices(float theta, int numHeightDivisions) {
        const float sinTheta = std::sin(theta);
        const float cosTheta = std::cos(theta);

        // Calculate cone surface normal
        const float slant_length = std::sqrt(radius * radius + height * height);
        const float ny = radius / slant_length;
        const float nr = height / slant_length;
        const glm::vec3 normal = glm::normalize(glm::vec3(cosTheta * nr, ny, sinTheta * nr));

        std::vector<unsigned int> indices;
        indices.reserve(numHeightDivisions + 1);

        // Create vertices from base to apex
        for (int h = 0; h <= numHeightDivisions; h++) {
            const float t = float(h) / float(numHeightDivisions);
            const float y = -height / 2.f + height * t;
            const float r = radius * (1.f - t);

            const glm::vec3 pos(r * cosTheta, y, r * sinTheta);
            const glm::vec2 texcoord(theta / glm::two_pi<float>(), t);

            indices.push_back(insertVertex(pos, normal, texcoord));
        }

        return indices;
    }

    void Cone::makeWedge(const std::vector<unsigned int>& currIndices,
                         const std::vector<unsigned int>& nextIndices) {
        const int numHeightDivisions = currIndices.size() - 1;

        // Build quads from base to apex
        for (int h = 0; h < numHeightDivisions; h++) {
            const unsigned int i0_curr = currIndices[h];      // Bottom-left
            const unsigned int i1_curr = currIndices[h + 1];  // Top-left
            const unsigned int i0_next = nextIndices[h];      // Bottom-right
            const unsigned int i1_next = nextIndices[h + 1];  // Top-right

            if (h == numHeightDivisions - 1) {
                // Apex triangle (i1_curr and i1_next are at the same position)
                insertIndex(i0_curr);
                insertIndex(i1_curr);
                insertIndex(i0_next);
            } else {
                // Regular quad split into two triangles
                // Triangle 1: bottom-left, top-right, bottom-right
                insertIndex(i0_curr);
                insertIndex(i1_next);
                insertIndex(i0_next);

                // Triangle 2: bottom-left, top-left, top-right
                insertIndex(i0_curr);
                insertIndex(i1_curr);
                insertIndex(i1_next);
            }
        }
    }

    void Cone::makeBaseCap(float thetaStep, int numWedges) {
        const glm::vec3 base_normal(0.f, -1.f, 0.f);
        const glm::vec3 base_center(0.f, -height / 2.f, 0.f);
        const unsigned int centerIdx = insertVertex(base_center, base_normal, glm::vec2(0.5f, 0.5f));

        // Create triangular fan for base
        for (int wedge = 0; wedge < numWedges; wedge++) {
            const float theta = float(wedge) * thetaStep;
            const float nextTheta = float(wedge + 1) * thetaStep;

            const glm::vec3 base_curr(radius * std::cos(theta), -height / 2.f, radius * std::sin(theta));
            const glm::vec3 base_next(radius * std::cos(nextTheta), -height / 2.f, radius * std::sin(nextTheta));

            const unsigned int currIdx = insertVertex(base_curr, base_normal,
                glm::vec2(0.5f + 0.5f * std::cos(theta), 0.5f + 0.5f * std::sin(theta)));
            const unsigned int nextIdx = insertVertex(base_next, base_normal,
                glm::vec2(0.5f + 0.5f * std::cos(nextTheta), 0.5f + 0.5f * std::sin(nextTheta)));

            // Base triangle (counter-clockwise from below)
            insertIndex(centerIdx);
            insertIndex(currIdx);
            insertIndex(nextIdx);
        }
    }
}