#include "Mesh.h"
#include "Graphics.h"
#include "MaterialConstants.h"
#include "../Util.h"
#include "../Debug.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include <stb_image.h>

#include "Primitive.h"
#include "Texture.h"


namespace gl {

    DrawShape Mesh::loadStaticShapeIndexed(const std::vector<float>& buffer_data, const std::vector<unsigned int>& indices) {
        constexpr int attribute_size = (3 + 3 + 2); // pos + normal + texcoord
        constexpr GLsizei stride = attribute_size * sizeof(float);

        DrawShape shape;
        GLuint vao, vbo, ebo;

        // Generate and bind VAO
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        // Generate and setup VBO
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, buffer_data.size() * sizeof(float), buffer_data.data(), GL_STATIC_DRAW);

        // Generate and setup EBO (must be done while VAO is bound)
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
        // Don't unbind GL_ELEMENT_ARRAY_BUFFER - it's part of VAO state!

        // Setup vertex attributes
        glEnableVertexAttribArray(0); // position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);

        glEnableVertexAttribArray(1); // normal
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));

        glEnableVertexAttribArray(2); // texcoord
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));

        // Unbind VAO (this preserves the EBO binding in the VAO)
        glBindVertexArray(0);
        // Now we can unbind the VBO
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Calculate bounding box from unique vertices
        glm::vec3 bmin(FLT_MAX);
        glm::vec3 bmax(-FLT_MAX);
        for (size_t i = 0; i < buffer_data.size(); i += attribute_size) {
            glm::vec3 v(buffer_data[i], buffer_data[i + 1], buffer_data[i + 2]);
            bmin = glm::min(bmin, v);
            bmax = glm::max(bmax, v);
        }

        shape.vao = vao;
        shape.vbo = vbo;
        shape.ebo = ebo;
        shape.numTriangles = indices.size() / 3;
        shape.min = bmin;
        shape.max = bmax;

        return shape;
    }

    DrawShape Mesh::loadPrimitive(const Primitive& primitive) {
        return loadStaticShapeIndexed(primitive.getBufferData(), primitive.getIndices());
    }

    constexpr unsigned int IMPORT_PRESET =  aiProcess_Triangulate |
                                            aiProcess_JoinIdenticalVertices |
                                            aiProcess_OptimizeMeshes |
                                            aiProcess_GenNormals |
                                            aiProcess_CalcTangentSpace |
                                            aiProcess_FlipUVs | // since OpenGL's UVs are flipped
                                            aiProcess_LimitBoneWeights; // Limit bone weights to 4 per vertex

    /**
     * Loads a static mesh from file, supporting various formats (OBJ, FBX, etc.)
     * Uses indexed rendering for better performance and memory efficiency.
     * @param filename - The file path to the mesh from project root, e.g "Resources/Models/model.obj"
     * @return DrawMesh struct containing the loaded mesh data
     */
    DrawMesh Mesh::loadStaticMesh(const char* filename) {
        auto directory = util::getDirectory(filename);
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(util::getPath(filename), IMPORT_PRESET);

        DrawMesh mesh;
        glm::vec3 min(std::numeric_limits<float>::max());
        glm::vec3 max(std::numeric_limits<float>::lowest());

        auto materials = Texture::loadSceneMaterials(scene, directory);

        for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
            const aiMesh* aimesh = scene->mMeshes[i];

            if (!aimesh->HasNormals() || !aimesh->HasPositions()) {
                continue;
            }

            // Build vertex buffer from unique vertices (indexed approach)
            std::vector<float> vertices;
            vertices.reserve(aimesh->mNumVertices * 8); // pos(3) + normal(3) + texcoord(2)

            for (unsigned int v = 0; v < aimesh->mNumVertices; v++) {
                const aiVector3D& pos = aimesh->mVertices[v];
                const aiVector3D& normal = aimesh->mNormals[v];
                const aiVector3D& texcoord = aimesh->HasTextureCoords(0) ?
                    aimesh->mTextureCoords[0][v] : aiVector3D(0.0f, 0.0f, 0.0f);

                // Position
                vertices.push_back(pos.x);
                vertices.push_back(pos.y);
                vertices.push_back(pos.z);

                // Normal
                vertices.push_back(normal.x);
                vertices.push_back(normal.y);
                vertices.push_back(normal.z);

                // Texcoord
                vertices.push_back(texcoord.x);
                vertices.push_back(texcoord.y);
            }

            // Build index buffer from faces
            std::vector<unsigned int> indices;
            indices.reserve(aimesh->mNumFaces * 3);

            for (unsigned int f = 0; f < aimesh->mNumFaces; f++) {
                const aiFace& face = aimesh->mFaces[f];
                for (unsigned int v = 0; v < face.mNumIndices; v++) {
                    indices.push_back(face.mIndices[v]);
                }
            }

            auto material_name = scene->mMaterials[aimesh->mMaterialIndex]->GetName().C_Str();

            DrawObject object;
            object.shape = loadStaticShapeIndexed(vertices, indices); // Use indexed loading
            object.material = materials[material_name];
            mesh.objects.push_back(object);
            min = glm::min(min, object.shape.min);
            max = glm::max(max, object.shape.max);
        }
        mesh.min = min;
        mesh.max = max;

        return mesh;
    }
}
