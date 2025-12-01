#pragma once
#include <vector>
#include <string>

#include "Graphics.h"
#include "glm/glm.hpp"

namespace gl {

#define MAX_BONES_PER_VERTEX 4
    using BoneIDs = std::array<unsigned int, MAX_BONES_PER_VERTEX>;
    using BoneWeights = std::array<float, MAX_BONES_PER_VERTEX>;

    struct Bone {
        std::string name;              // Bone name from FBX file
        unsigned int id;                        // Unique bone index
        int parent_id;                 // Parent bone index (-1 for root)
        glm::mat4 offset_matrix;       // Inverse bind pose - transforms from mesh space to bone space
        glm::mat4 local_transform;   // Local transform relative to parent bone
        std::vector<unsigned int> children;     // Indices of child bones

        Bone(const std::string& bone_name, const unsigned int bone_id, const int parent,
            const glm::mat4& offset, const glm::mat4& local_transform) :
        name(bone_name),
        id(bone_id),
        parent_id(parent),
        offset_matrix(offset),
        local_transform(local_transform) {}

        void addChild(const Bone& child) {
            children.push_back(child.id);
        }
    };

    struct Skeleton {
        public:
        void addBone(const std::string& bone_name, const unsigned int current_id, const int parent_id,
            const glm::mat4& offset_matrix, const glm::mat4& local_transform);
        void updateBoneMatrices();

        void traverseBoneHierarchy(const unsigned int bone_id, const glm::mat4& parent_transform);
        unsigned int num_bones_;
        std::vector<Bone> bones_;
        std::unordered_map<std::string, unsigned int> bone_map_;
        std::vector<glm::mat4> bone_matrices_;
        glm::mat4 global_inverse_;
    };

    struct SkinnedMesh {
        DrawMesh draw_mesh;
        Skeleton skeleton;
    };


    struct Animation {

    };




    class SkeletalMesh {
    public:


        static SkinnedMesh loadFbx(const char* filename);




    private:

        static DrawShape loadSkinnedShape(
            const std::vector<glm::vec3>& positions,
            const std::vector<glm::vec3>& normals,
            const std::vector<glm::vec2>& texcoords,
            const std::vector<BoneIDs>& bone_ids,
            const std::vector<BoneWeights>& bone_weights);


        std::vector<glm::vec3> vertex_data_;
        std::vector<glm::ivec4> bone_ids_; // Bone IDs for each vertex
        std::vector<glm::vec4> bone_weights_; // Bone weights for each vertex



    };
}
