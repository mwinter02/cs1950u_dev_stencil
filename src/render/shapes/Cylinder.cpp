#include "Cylinder.h"

namespace gl {
    Cylinder::Cylinder(int radial_subdivisions, int height_subdivisions) {
        param_1 = std::max(1, height_subdivisions);    // Height subdivisions
        param_2 = std::max(3, radial_subdivisions);    // Radial subdivisions
        Cylinder::makeShape();
    }

    void Cylinder::makeShape() {
        const int numRadialSegments = std::max(3, param_2);
        const int numHeightDivisions = std::max(1, param_1);
        const float thetaStep = glm::radians(360.f / float(numRadialSegments));

        // Create vertex rings: (numRadialSegments + 1) columns for wrapping × (numHeightDivisions + 1) rows
        std::vector<std::vector<unsigned int>> vertexRings(numHeightDivisions + 1);

        // Generate vertices for each horizontal ring
        for (int h = 0; h <= numHeightDivisions; h++) {
            const float t = float(h) / float(numHeightDivisions);
            const float y = -height / 2.f + height * t;

            // Create ring with extra column for seamless wrapping
            std::vector<unsigned int> ring;
            ring.reserve(numRadialSegments + 1);

            unsigned int firstVertexIdx = 0;  // Store first vertex index for wrapping

            for (int seg = 0; seg <= numRadialSegments; seg++) {
                if (seg < numRadialSegments) {
                    const float theta = float(seg) * thetaStep;
                    unsigned int vertexIdx = createRingVertices(y, theta, h, numHeightDivisions)[0];
                    ring.push_back(vertexIdx);

                    if (seg == 0) {
                        firstVertexIdx = vertexIdx;  // Save first vertex for wrapping
                    }
                } else {
                    // Last column wraps to first vertex for seamless texture
                    ring.push_back(firstVertexIdx);
                }
            }

            vertexRings[h] = ring;
        }

        // Build triangles for cylinder side
        for (int h = 0; h < numHeightDivisions; h++) {
            makeSegment(vertexRings[h], vertexRings[h + 1]);
        }

        // Build top and bottom caps
        makeCap(-height / 2.f, glm::vec3(0, -1, 0), false);  // Bottom cap
        makeCap(height / 2.f, glm::vec3(0, 1, 0), true);     // Top cap
    }

    std::vector<unsigned int> Cylinder::createRingVertices(float y, float theta, int heightIndex, int totalHeightDivisions) {
        const float sinTheta = std::sin(theta);
        const float cosTheta = std::cos(theta);

        // Cylinder side normal points radially outward (horizontal)
        const glm::vec3 normal = glm::normalize(glm::vec3(cosTheta, 0.f, sinTheta));

        // Position on cylinder surface
        const glm::vec3 pos(radius * cosTheta, y, radius * sinTheta);

        // UV coordinates (standard cylinder unwrap)
        const float u = theta / (2.0f * 3.14159265359f);  // Wrap around circumference (0 to 1)
        const float v = float(heightIndex) / float(totalHeightDivisions);  // Height (0 to 1)
        const glm::vec2 texcoord(u, v);

        std::vector<unsigned int> indices;
        indices.push_back(insertVertex(pos, normal, texcoord));
        return indices;
    }

    void Cylinder::makeSegment(const std::vector<unsigned int>& bottomRing,
                                const std::vector<unsigned int>& topRing) {
        const int numSegments = bottomRing.size() - 1;  // -1 because last vertex is duplicate of first

        for (int seg = 0; seg < numSegments; seg++) {
            const unsigned int bottomLeft = bottomRing[seg];
            const unsigned int bottomRight = bottomRing[seg + 1];
            const unsigned int topLeft = topRing[seg];
            const unsigned int topRight = topRing[seg + 1];

            // Quad split into two triangles (counter-clockwise winding)
            // Triangle 1: bottom-left, top-left, top-right
            insertIndex(bottomLeft);
            insertIndex(topLeft);
            insertIndex(topRight);

            // Triangle 2: bottom-left, top-right, bottom-right
            insertIndex(bottomLeft);
            insertIndex(topRight);
            insertIndex(bottomRight);
        }
    }

    void Cylinder::makeCap(float y, const glm::vec3& normal, bool isTop) {
        const int numRadialSegments = std::max(3, param_2);
        const float thetaStep = glm::radians(360.f / float(numRadialSegments));

        // Center vertex of the cap
        const glm::vec3 center(0.f, y, 0.f);
        const glm::vec2 centerUV(0.5f, 0.5f);
        const unsigned int centerIdx = insertVertex(center, normal, centerUV);

        // Create triangular fan for cap
        for (int seg = 0; seg < numRadialSegments; seg++) {
            const float currTheta = float(seg) * thetaStep;
            const float nextTheta = float(seg + 1) * thetaStep;

            // Positions on the rim
            const glm::vec3 currPos(radius * std::cos(currTheta), y, radius * std::sin(currTheta));
            const glm::vec3 nextPos(radius * std::cos(nextTheta), y, radius * std::sin(nextTheta));

            // UV coordinates (circular mapping)
            const glm::vec2 currUV(0.5f + 0.5f * std::cos(currTheta), 0.5f + 0.5f * std::sin(currTheta));
            const glm::vec2 nextUV(0.5f + 0.5f * std::cos(nextTheta), 0.5f + 0.5f * std::sin(nextTheta));

            const unsigned int currIdx = insertVertex(currPos, normal, currUV);
            const unsigned int nextIdx = insertVertex(nextPos, normal, nextUV);

            // Triangle winding depends on which cap (top vs bottom)
            if (isTop) {
                // Top cap: counter-clockwise from above (looking down)
                insertIndex(centerIdx);
                insertIndex(nextIdx);
                insertIndex(currIdx);
            } else {
                // Bottom cap: counter-clockwise from below (looking up)
                insertIndex(centerIdx);
                insertIndex(currIdx);
                insertIndex(nextIdx);
            }
        }
    }
}

