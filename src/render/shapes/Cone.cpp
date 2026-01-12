#include "Cone.h"

namespace gl {
    Cone::Cone(int quality) {
        param_1 = 1;
        param_2 = quality;
        Cone::makeShape();
    }

    void Cone::makeShape() {
        int numDivisions = std::max(3, param_2);
        float thetaStep = glm::radians(360.f / float(numDivisions));
        for (int i = 0; i < numDivisions; i++) {
            float currentTheta = float(i) * thetaStep;
            float nextTheta = float(i + 1) * thetaStep;
            makeWedge(currentTheta, nextTheta);
        }
    }

    void Cone::makeWedge(float curr_theta, float next_theta) {
        const int numDivisions = std::max(1, param_1);

        const float sinCurr = std::sin(curr_theta);
        const float cosCurr = std::cos(curr_theta);
        const float sinNext = std::sin(next_theta);
        const float cosNext = std::cos(next_theta);

        const glm::vec3 base_center(0.f, -height / 2.f, 0.f);
        const glm::vec3 base_curr(radius * cosCurr, -height / 2.f, radius * sinCurr);
        const glm::vec3 base_next(radius * cosNext, -height / 2.f, radius * sinNext);

        // Cone surface normal calculation constants
        // For smooth shading, each vertex gets its own normal based on its radial position
        const float slant_length = std::sqrt(radius * radius + height * height);
        const float ny = radius / slant_length;  // Vertical component of normal
        const float nr = height / slant_length;  // Radial component of normal

        const glm::vec3 base_normal(0.f, -1.f, 0.f);

        // Lambda to calculate normal at any point on the cone surface
        auto calculateNormal = [&](float cosTheta, float sinTheta) -> glm::vec3 {
            return glm::normalize(glm::vec3(
                cosTheta * nr,
                ny,
                sinTheta * nr
            ));
        };

        // Build side surface with vertical subdivisions
        for (int i = 0; i < numDivisions; i++) {
            const float t0 = float(i) / float(numDivisions);
            const float t1 = float(i + 1) / float(numDivisions);

            const float y0 = -height / 2.f + height * t0;
            const float y1 = -height / 2.f + height * t1;
            const float r0 = radius * (1.f - t0);
            const float r1 = radius * (1.f - t1);

            const glm::vec3 p0_curr(r0 * cosCurr, y0, r0 * sinCurr);
            const glm::vec3 p0_next(r0 * cosNext, y0, r0 * sinNext);
            const glm::vec3 p1_curr(r1 * cosCurr, y1, r1 * sinCurr);
            const glm::vec3 p1_next(r1 * cosNext, y1, r1 * sinNext);

            // Calculate normals for each vertex position
            const glm::vec3 n0_curr = calculateNormal(cosCurr, sinCurr);
            const glm::vec3 n0_next = calculateNormal(cosNext, sinNext);
            const glm::vec3 n1_curr = n0_curr; // Same radial position
            const glm::vec3 n1_next = n0_next; // Same radial position

            // For last iteration, r1 is nearly 0 (apex), only create one triangle
            if (i == numDivisions - 1) {
                // Single triangle to apex (counter-clockwise winding)
                insertBufferData(p0_curr, n0_curr, glm::vec2(0.f, t0));
                insertBufferData(p1_curr, n1_curr, glm::vec2(0.5f, t1));
                insertBufferData(p0_next, n0_next, glm::vec2(1.f, t0));
            } else {
                // Quad made of two triangles (counter-clockwise winding)
                // Triangle 1: bottom-left, bottom-right, top-right
                insertBufferData(p0_curr, n0_curr, glm::vec2(0.f, t0));
                insertBufferData(p0_next, n0_next, glm::vec2(1.f, t0));
                insertBufferData(p1_next, n1_next, glm::vec2(1.f, t1));

                // Triangle 2: bottom-left, top-right, top-left
                insertBufferData(p0_curr, n0_curr, glm::vec2(0.f, t0));
                insertBufferData(p1_next, n1_next, glm::vec2(1.f, t1));
                insertBufferData(p1_curr, n1_curr, glm::vec2(0.f, t1));
            }
        }

        // Base cap (counter-clockwise winding when viewed from below)
        insertBufferData(base_center, base_normal, glm::vec2(0.5f, 0.5f));
        insertBufferData(base_curr, base_normal, glm::vec2(0.5f + 0.5f * cosCurr, 0.5f + 0.5f * sinCurr));
        insertBufferData(base_next, base_normal, glm::vec2(0.5f + 0.5f * cosNext, 0.5f + 0.5f * sinNext));
    }
}