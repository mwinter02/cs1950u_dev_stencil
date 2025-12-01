#include "SkeletalMesh.h"

#include "Mesh.h"
#include "../Debug.h"
#include "../Util.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"


namespace gl {


    std::string findRootBone(aiNode* curr_node, const std::unordered_set<std::string>& bone_map) {
        if (bone_map.contains(curr_node->mName.C_Str())) {
            return curr_node->mName.C_Str();
        }
        for (size_t i = 0; i < curr_node->mNumChildren; i++) {
            auto result = findRootBone(curr_node->mChildren[i], bone_map);
            if (!result.empty()) {
                return result;
            }
        }
        debug::error("Failed to find root bone");
        return "";
    }

    glm::mat4 getBoneMatrix(const std::string& bone_name, const aiScene* scene) {
        if (auto bone = scene->findBone(aiString(bone_name))) {
            return util::aiToGlmMat4(bone->mOffsetMatrix);
        }

        debug::error("Failed to find bone " + bone_name);
        return {1.0f};
    }

    void constructSkeleton(Skeleton& skeleton, aiNode* curr_node, const aiScene* scene, int parent_id) {
        aiBone* ai_bone = scene->findBone(curr_node->mName);
        if (ai_bone) {
            const int current_id = (int) skeleton.bones_.size();
            const glm::mat4 offset_matrix = util::aiToGlmMat4(ai_bone->mOffsetMatrix);
            const glm::mat4 local_transform = util::aiToGlmMat4(curr_node->mTransformation);

            skeleton.addBone(curr_node->mName.C_Str(), current_id, parent_id, offset_matrix, local_transform);

            for (size_t i = 0; i < curr_node->mNumChildren; i++) {
                constructSkeleton(skeleton, curr_node->mChildren[i], scene, current_id);
            }
        }
        else { // Current node not a bone, continue to children
            for (size_t i = 0; i < curr_node->mNumChildren; i++) {
                constructSkeleton(skeleton, curr_node->mChildren[i], scene, parent_id);
            }
        }
    }

    constexpr unsigned int SKINNED_IMPORT_PRESET =  aiProcess_Triangulate |
                                                    aiProcess_JoinIdenticalVertices |
                                                    aiProcess_OptimizeMeshes |
                                                    aiProcess_GenSmoothNormals |
                                                    aiProcess_CalcTangentSpace |
                                                    aiProcess_PopulateArmatureData | // Create bone hierarchy
                                                    aiProcess_FlipUVs | // since OpenGL's UVs are flipped
                                                    aiProcess_LimitBoneWeights; // Limit bone weights to 4 per vertex


    Skeleton loadSkeleton(const aiScene* scene) {
        auto root_transform = util::aiToGlmMat4(scene->mRootNode->mTransformation);

        Skeleton skeleton;
        skeleton.global_inverse_ = glm::inverse(root_transform);
        std::unordered_set<std::string> bone_names;

        for (int i=0; i < scene->mNumMeshes; i++) {
            const aiMesh* aimesh = scene->mMeshes[i];
            if (!aimesh->HasBones()) { continue; }


            for (size_t b = 0; b < aimesh->mNumBones; b++) {
                const aiBone* bone = aimesh->mBones[b];

                std::string bone_name = bone->mName.C_Str();
                if (!bone_names.contains(bone_name)) {
                    bone_names.insert(bone_name);
                }
            }
        }
        std::string root_name = findRootBone(scene->mRootNode, bone_names);
        auto node = scene->mRootNode->FindNode(aiString(root_name));
        constructSkeleton(skeleton, node, scene, -1);
        return skeleton;
    }

    void Skeleton::addBone(const std::string& bone_name, const unsigned int current_id, const int parent_id,
        const glm::mat4& offset_matrix, const glm::mat4& local_transform) {

        const auto to_add = Bone(bone_name, current_id, parent_id, offset_matrix, local_transform);
        bones_.push_back(to_add);
        bone_matrices_.push_back(offset_matrix);
        bone_map_[to_add.name] = current_id;
        num_bones_ = (unsigned int) bones_.size();
        if (parent_id != -1) bones_[parent_id].addChild(to_add);
    }


    void Skeleton::traverseBoneHierarchy(const unsigned int bone_id, const glm::mat4& parent_transform) {
        const auto& bone = bones_[bone_id];

        // Accumulate transforms: parent's global transform × this bone's local transform
        const glm::mat4 global_transform = parent_transform * bone.local_transform;

        // Calculate final bone matrix for shader: GlobalInverse × GlobalTransform × OffsetMatrix
        bone_matrices_[bone_id] = global_transform * bone.offset_matrix;

        // Recursively update all children with this bone's global transform as their parent
        for (const auto& child_id : bone.children) {
            traverseBoneHierarchy(child_id, global_transform);
        }
    }

    void Skeleton::updateBoneMatrices() {
        // Ensure bone_matrices_ is sized correctly
        if (bone_matrices_.size() != num_bones_) {
            bone_matrices_.resize(num_bones_, glm::mat4(1.0f));
        }

        // Find and traverse from all root bones (those with no parent)
        for (unsigned int i = 0; i < bones_.size(); i++) {
            if (bones_[i].parent_id == -1) {
                // Start traversal from root with identity parent transform
                traverseBoneHierarchy(i, glm::mat4(1.0f));
                break;
            }
        }
    }

    SkinnedMesh SkeletalMesh::loadFbx(const char* filename) {
        auto directory = util::getDirectory(filename);

        Assimp::Importer importer;

        // aiProcess_LimitBoneWeights; // Limit bone weights to 4 per vertex


        auto path = util::getPath(filename);
        const aiScene* scene = importer.ReadFile(path,SKINNED_IMPORT_PRESET);     // Calculate tangent space for normal mapping

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            debug::error("Failed to load FBX: " + std::string(importer.GetErrorString()));
            return {};
        }

        auto skeleton = loadSkeleton(scene);
        skeleton.updateBoneMatrices();  // Calculate bone matrices for bind pose

        auto materials = Texture::loadSceneMaterials(scene, directory);
        DrawMesh mesh;
        for (size_t i=0; i<scene->mNumMeshes; i++) {
            const aiMesh* aimesh = scene->mMeshes[i];
            if (!aimesh->HasBones()) { continue; }



            std::vector<glm::vec3> vertices(aimesh->mNumVertices);
            std::vector<BoneIDs> bone_ids(aimesh->mNumVertices, BoneIDs{});
            std::vector<BoneWeights> bone_weights(aimesh->mNumVertices, BoneWeights{});



            for (size_t b = 0; b < aimesh->mNumBones; b++) { // populate bone data
                const aiBone* bone = aimesh->mBones[b];
                auto bone_id = skeleton.bone_map_[bone->mName.C_Str()];

                for (size_t w = 0; w < bone->mNumWeights; w++) {
                    const aiVertexWeight& weight = bone->mWeights[w];
                    const size_t vertex_id = weight.mVertexId;
                    for (size_t j = 0; j < MAX_BONES_PER_VERTEX; j++) {
                        if (bone_weights[vertex_id][j] == 0.0f) {
                            bone_ids[vertex_id][j] = bone_id;
                            bone_weights[vertex_id][j] = weight.mWeight;
                            break;
                        }
                    }
                }
            }

            // Normalize bone weights, default to 1.0 for root bone if no weights
            for (auto& b_w : bone_weights) {
                if (b_w == BoneWeights{}) {
                    b_w[0] = 1.0f;
                } else {
                    float sum = 0;
                    for (auto w : b_w) sum += w;
                    if (sum != 1.0f) for (auto& w : b_w) w /= sum; // normalize
                }
            }

            // Separate vectors for each attribute type
            std::vector<glm::vec3> positions;
            std::vector<glm::vec3> normals;
            std::vector<glm::vec2> texcoords;
            std::vector<BoneIDs> face_bone_ids;
            std::vector<BoneWeights> face_bone_weights;

            positions.reserve(aimesh->mNumFaces * 3);
            normals.reserve(aimesh->mNumFaces * 3);
            texcoords.reserve(aimesh->mNumFaces * 3);
            face_bone_ids.reserve(aimesh->mNumFaces * 3);
            face_bone_weights.reserve(aimesh->mNumFaces * 3);

            for (int f = 0; f < aimesh->mNumFaces; f++) {
                const aiFace& face = aimesh->mFaces[f];
                for (int v = 0; v < face.mNumIndices; v++) {
                    auto index = face.mIndices[v];
                    const aiVector3D& pos = aimesh->mVertices[index];
                    const aiVector3D& normal = aimesh->mNormals[index];
                    const aiVector3D& texcoord = aimesh->HasTextureCoords(0) ? aimesh->mTextureCoords[0][index] : aiVector3D(0.0f, 0.0f, 0.0f);

                    vertices[index] = glm::vec3(pos.x, pos.y, pos.z);

                    positions.push_back(glm::vec3(pos.x, pos.y, pos.z));
                    normals.push_back(glm::vec3(normal.x, normal.y, normal.z));
                    texcoords.push_back(glm::vec2(texcoord.x, texcoord.y));
                    face_bone_ids.push_back(bone_ids[index]);
                    face_bone_weights.push_back(bone_weights[index]);
                }
            }

            auto material_name = scene->mMaterials[aimesh->mMaterialIndex]->GetName().C_Str();
            DrawObject object;
            object.shape = loadSkinnedShape(positions, normals, texcoords, face_bone_ids, face_bone_weights);
            object.material = materials[material_name];
            mesh.objects.push_back(object);

        }


        skeleton.updateBoneMatrices();
        return {mesh, skeleton};
    }

    DrawShape SkeletalMesh::loadSkinnedShape(
        const std::vector<glm::vec3>& positions,
        const std::vector<glm::vec3>& normals,
        const std::vector<glm::vec2>& texcoords,
        const std::vector<BoneIDs>& bone_ids,
        const std::vector<BoneWeights>& bone_weights) {

        DrawShape shape;
        GLuint vao;
        GLuint vbo_pos, vbo_normal, vbo_texcoord, vbo_bone_ids, vbo_bone_weights;

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        // Position buffer (attribute 0)
        glGenBuffers(1, &vbo_pos);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_pos);
        glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(glm::vec3), positions.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

        // Normal buffer (attribute 1)
        glGenBuffers(1, &vbo_normal);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_normal);
        glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

        // Texcoord buffer (attribute 2)
        glGenBuffers(1, &vbo_texcoord);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_texcoord);
        glBufferData(GL_ARRAY_BUFFER, texcoords.size() * sizeof(glm::vec2), texcoords.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

        // Bone IDs buffer (attribute 3) - PROPER INTEGER HANDLING
        glGenBuffers(1, &vbo_bone_ids);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_bone_ids);
        glBufferData(GL_ARRAY_BUFFER, bone_ids.size() * sizeof(BoneIDs), bone_ids.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(3);
        glVertexAttribIPointer(3, 4, GL_UNSIGNED_INT, 0, nullptr);  // Use GL_UNSIGNED_INT

        // Bone weights buffer (attribute 4)
        glGenBuffers(1, &vbo_bone_weights);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_bone_weights);
        glBufferData(GL_ARRAY_BUFFER, bone_weights.size() * sizeof(BoneWeights), bone_weights.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 0, nullptr);

        glBindVertexArray(0);

        // Calculate bounding box
        glm::vec3 bmin(FLT_MAX);
        glm::vec3 bmax(-FLT_MAX);
        for (const auto& pos : positions) {
            bmin = glm::min(bmin, pos);
            bmax = glm::max(bmax, pos);
        }

        shape.vao = vao;
        shape.vbo = vbo_pos;  // Store primary VBO (we could also store all VBOs if needed)
        shape.numTriangles = positions.size() / 3;
        shape.min = bmin;
        shape.max = bmax;

        return shape;
    }
}