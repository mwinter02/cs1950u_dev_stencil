#include "Sphere.h"

namespace gl {
    Sphere::Sphere(int latitudinal_subdivisions, int longitudinal_subdivisions) {
        param_1 = std::max(2, latitudinal_subdivisions);    // Latitude divisions (vertical)
        param_2 = std::max(3, longitudinal_subdivisions);   // Longitude divisions (horizontal)
        Sphere::makeShape();
    }

    void Sphere::makeShape() {
        const int numLatitudeDivisions = std::max(2, param_1);   // Vertical slices
        const int numLongitudeDivisions = std::max(3, param_2);  // Horizontal slices

        // Create vertex grid: (numLatitudeDivisions + 1) rows × (numLongitudeDivisions + 1) columns
        // Extra column for seamless texture wrapping at longitude 0/360
        std::vector<std::vector<unsigned int>> vertexGrid(numLatitudeDivisions + 1);

        // Generate vertices using spherical coordinates
        for (int lat = 0; lat <= numLatitudeDivisions; lat++) {
            vertexGrid[lat].reserve(numLongitudeDivisions + 1);

            // Latitude angle: 0 (south pole) to π (north pole)
            const float theta = float(lat) / float(numLatitudeDivisions) * 3.14159265359f;
            const float sinTheta = std::sin(theta);
            const float cosTheta = std::cos(theta);

            unsigned int firstVertexIdx = 0;  // Store first vertex for wrapping

            for (int lon = 0; lon <= numLongitudeDivisions; lon++) {
                if (lon < numLongitudeDivisions) {
                    // Longitude angle: 0 to 2π
                    const float phi = float(lon) / float(numLongitudeDivisions) * 2.0f * 3.14159265359f;
                    const float sinPhi = std::sin(phi);
                    const float cosPhi = std::cos(phi);

                    // Spherical to Cartesian conversion
                    const float x = radius * sinTheta * cosPhi;
                    const float y = radius * cosTheta;
                    const float z = radius * sinTheta * sinPhi;

                    const glm::vec3 position(x, y, z);

                    // Normal for a sphere is the normalized position vector
                    const glm::vec3 normal = glm::normalize(position);

                    // UV coordinates (equirectangular mapping)
                    const float u = float(lon) / float(numLongitudeDivisions);
                    const float v = float(lat) / float(numLatitudeDivisions);
                    const glm::vec2 texcoord(u, v);

                    unsigned int vertexIdx = insertVertex(position, normal, texcoord);
                    vertexGrid[lat].push_back(vertexIdx);

                    if (lon == 0) {
                        firstVertexIdx = vertexIdx;
                    }
                } else {
                    // Last column wraps to first vertex for seamless texture
                    vertexGrid[lat].push_back(firstVertexIdx);
                }
            }
        }

        // Build triangles (skip first and last latitude rows for poles - handle them separately)
        for (int lat = 0; lat < numLatitudeDivisions; lat++) {
            for (int lon = 0; lon < numLongitudeDivisions; lon++) {
                const unsigned int current = vertexGrid[lat][lon];
                const unsigned int next = vertexGrid[lat][lon + 1];
                const unsigned int below = vertexGrid[lat + 1][lon];
                const unsigned int belowNext = vertexGrid[lat + 1][lon + 1];

                if (lat == 0) {
                    // Top pole - only one triangle per segment (degenerate quad)
                    insertIndex(current);
                    insertIndex(belowNext);
                    insertIndex(below);
                } else if (lat == numLatitudeDivisions - 1) {
                    // Bottom pole - only one triangle per segment (degenerate quad)
                    insertIndex(current);
                    insertIndex(next);
                    insertIndex(belowNext);
                } else {
                    // Regular quad - split into two triangles
                    // Triangle 1: current, next, belowNext
                    insertIndex(current);
                    insertIndex(next);
                    insertIndex(belowNext);

                    // Triangle 2: current, belowNext, below
                    insertIndex(current);
                    insertIndex(belowNext);
                    insertIndex(below);
                }
            }
        }
    }
}

