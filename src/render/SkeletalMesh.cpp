#include "SkeletalMesh.h"

#include <ranges>

#include "Mesh.h"
#include "../Debug.h"
#include "../Util.h"
#include "assimp/cimport.h"
#include "assimp/Exporter.hpp"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"


namespace gl {

    // Helper function to find the index of the keyframe just before or at the given time
    template<typename T>
    int findKeyframeIndex(const std::vector<Keyframe<T>>& keys, double time) {
        for (int i = 0; i < keys.size() - 1; i++) {
            if (time < keys[i + 1].time) {
                return i;
            }
        }
        return keys.size() - 1;
    }

    // Interpolate position (linear interpolation)
    glm::vec3 interpolatePosition(const std::vector<PositionKey>& keys, double time) {
        if (keys.empty()) {
            return glm::vec3(0.0f);
        }
        if (keys.size() == 1) {
            return keys[0].value;
        }

        int index = findKeyframeIndex(keys, time);
        int next_index = index + 1;

        // Clamp to last keyframe if we're past the end
        if (next_index >= keys.size()) {
            return keys[index].value;
        }

        const auto& key1 = keys[index];
        const auto& key2 = keys[next_index];

        double delta_time = key2.time - key1.time;
        float factor = static_cast<float>((time - key1.time) / delta_time);

        // Linear interpolation
        return glm::mix(key1.value, key2.value, factor);
    }

    // Interpolate rotation (spherical linear interpolation for smooth rotation)
    glm::quat interpolateRotation(const std::vector<RotationKey>& keys, double time) {
        if (keys.empty()) {
            return glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // Identity quaternion
        }
        if (keys.size() == 1) {
            return keys[0].value;
        }

        int index = findKeyframeIndex(keys, time);
        int next_index = index + 1;

        // Clamp to last keyframe if we're past the end
        if (next_index >= keys.size()) {
            return keys[index].value;
        }

        const auto& key1 = keys[index];
        const auto& key2 = keys[next_index];

        double delta_time = key2.time - key1.time;
        float factor = static_cast<float>((time - key1.time) / delta_time);

        // Spherical linear interpolation (slerp) for smooth rotation
        return glm::slerp(key1.value, key2.value, factor);
    }

    // Interpolate scale (linear interpolation)
    glm::vec3 interpolateScale(const std::vector<ScaleKey>& keys, double time) {
        if (keys.empty()) {
            return glm::vec3(1.0f); // Default scale
        }
        if (keys.size() == 1) {
            return keys[0].value;
        }

        int index = findKeyframeIndex(keys, time);
        int next_index = index + 1;

        // Clamp to last keyframe if we're past the end
        if (next_index >= keys.size()) {
            return keys[index].value;
        }

        const auto& key1 = keys[index];
        const auto& key2 = keys[next_index];

        double delta_time = key2.time - key1.time;
        float factor = static_cast<float>((time - key1.time) / delta_time);

        // Linear interpolation
        return glm::mix(key1.value, key2.value, factor);
    }

    glm::mat4 AnimationChannel::calculateTransform(double animation_time) const {
        const glm::vec3 position = interpolatePosition(position_keys, animation_time);
        const glm::quat rotation = interpolateRotation(rotation_keys, animation_time);
        const glm::vec3 scale = interpolateScale(scale_keys, animation_time);

        return glm::translate(glm::mat4(1.0f), position)
             * glm::mat4_cast(rotation)
             * glm::scale(glm::mat4(1.0f), scale);
    }

    std::string findRootBone(aiNode* curr_node, const std::unordered_set<std::string>& bone_map) {
        if (bone_map.contains(curr_node->mName.C_Str())) {
            if (auto parent = curr_node->mParent) {
                if (bone_map.contains(parent->mName.C_Str())) {
                    // Not root, continue searching
                    return findRootBone(parent, bone_map);
                }
            }
            // No parent, or parent not a bone: this is root
            return curr_node->mName.C_Str();
        }
        // for (size_t i = 0; i < curr_node->mNumChildren; i++) {
        //     auto result = findRootBone(curr_node->mChildren[i], bone_map);
        //     if (!result.empty()) {
        //         return result;
        //     }
        // }
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

    void createHierarchy(Skeleton& skeleton, const aiScene* scene, const std::unordered_set<std::string>& bone_names) {

        for (auto bone_name : bone_names) {
            auto node = scene->mRootNode->FindNode(aiString(bone_name));
            if (node && node->mParent) {
                std::string parent_name = node->mParent->mName.C_Str();
                if (bone_names.contains(parent_name)) {
                    int bone_id = skeleton.bone_map_[bone_name];
                    int parent_id = skeleton.bone_map_[parent_name];
                    skeleton.bones_[bone_id].parent_id = parent_id;
                    skeleton.bones_[parent_id].addChild(skeleton.bones_[bone_id]);
                }
            }
        }

    }

    void constructSkeleton(Skeleton& skeleton, aiNode* curr_node, const aiScene* scene,
                          int parent_id, const glm::mat4& parent_global_transform) {
        aiBone* ai_bone = scene->findBone(curr_node->mName);
        const glm::mat4 node_local = util::aiToGlmMat4(curr_node->mTransformation);
        const glm::mat4 node_global = parent_global_transform * node_local;
        const int current_id = (int) skeleton.bones_.size();
        if (ai_bone) {

            const glm::mat4 offset_matrix = util::aiToGlmMat4(ai_bone->mOffsetMatrix);
            // The offset matrix is the INVERSE of the bind pose global transform
            // So the bind pose global transform is the inverse of the offset matrix
            const glm::mat4 bind_pose_global = glm::inverse(offset_matrix);
            // Determine the local transform to store
            glm::mat4 local_transform;
            if (parent_id == -1) {
                // Root bone: local_transform IS the bind pose global transform
                local_transform = bind_pose_global;
            } else {
                // Child bone: extract local transform by dividing out parent's bind pose
                // child_global = parent_global × child_local
                // child_local = inverse(parent_global) × child_global
                const glm::mat4& parent_bind_global = glm::inverse(skeleton.bones_[parent_id].offset_matrix);
                local_transform = glm::inverse(parent_bind_global) * bind_pose_global;
            }

            skeleton.addBone(curr_node->mName.C_Str(), current_id, parent_id, offset_matrix, local_transform);

            // Continue to children with this bone as parent (use node_global for hierarchy traversal)
            for (size_t i = 0; i < curr_node->mNumChildren; i++) {
                constructSkeleton(skeleton, curr_node->mChildren[i], scene, current_id, node_global);
            }
        }
        else {
            skeleton.addBone(curr_node->mName.C_Str(), current_id, parent_id,
                             glm::mat4(1.0f), node_local, true); // Virtual bone
            for (size_t i = 0; i < curr_node->mNumChildren; i++) {
                constructSkeleton(skeleton, curr_node->mChildren[i], scene, current_id, node_global);
            }
        }
    }

    constexpr unsigned int SKINNED_IMPORT_PRESET =  aiProcess_Triangulate |
                                                    aiProcess_JoinIdenticalVertices |
                                                    aiProcess_OptimizeMeshes |
                                                    aiProcess_GenNormals |
                                                    aiProcess_CalcTangentSpace |
                                                    // aiProcess_PopulateArmatureData | // Create bone hierarchy
                                                    aiProcess_FlipUVs | // since OpenGL's UVs are flipped
                                                    aiProcess_LimitBoneWeights; // Limit bone weights to 4 per vertex


    Skeleton loadSkeleton(const aiScene* scene) {
        auto root_transform = util::aiToGlmMat4(scene->mRootNode->mTransformation);

        Skeleton skeleton;
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

        auto start_bone = scene->mRootNode->FindNode(aiString(*bone_names.begin()));
        std::string root_name = findRootBone(start_bone, bone_names);
        auto node = scene->mRootNode->FindNode(aiString(root_name));
        auto name = node->mParent->mName;


        constructSkeleton(skeleton, scene->mRootNode, scene, -1, glm::mat4(1.0f));
        auto x = skeleton.bones_.size();


        return skeleton;
    }

    void Skeleton::addBone(const std::string& bone_name, const unsigned int current_id, const int parent_id,
        const glm::mat4& offset_matrix, const glm::mat4& local_transform, const bool is_virtual) {

        const auto to_add = Bone(bone_name, current_id, parent_id, offset_matrix, local_transform, is_virtual);
        bones_.push_back(to_add);
        bone_matrices_.push_back(offset_matrix);
        bone_map_[to_add.name] = current_id;
        num_bones_ = (unsigned int) bones_.size();
        if (parent_id != -1) bones_[parent_id].addChild(to_add);
    }


    void Skeleton::traverseBoneHierarchy(const unsigned int bone_id, const glm::mat4& parent_transform) {
        auto& bone = bones_[bone_id];

        // Accumulate transforms: parent's global transform × this bone's local transform
        const glm::mat4 global_transform = parent_transform * bone.local_transform;

        // Calculate final bone matrix for shader
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

    void Skeleton::setCurrentAnimation(size_t index) {
        current_animation_ = &animations_[index];
    }

    void Skeleton::setCurrentAnimation(const std::string& animation_name) {
        if (!animations_name_to_index.contains(animation_name)) {
            debug::error("Animation " + animation_name + " not found in skeleton.");
            return;
        }
        current_animation_ = &animations_[animations_name_to_index.at(animation_name)];
    }

    void Skeleton::playCurrentAnimation(double time_since_previous) {
        if (current_animation_) {
            current_animation_->time_in_seconds += time_since_previous;
            const double time_in_ticks = current_animation_->time_in_seconds * current_animation_->ticks_per_second;
            const double animation_time = fmod(time_in_ticks, current_animation_->duration);
            // Update each bone's local transform based on animation channels
            for (auto& [bone_id, channel] : current_animation_->channels) {
                bones_[bone_id].local_transform = channel.calculateTransform(animation_time);
            }
        }

        updateBoneMatrices();
    }

    void Skeleton::resetToBindPose() {
        for (auto& bone : bones_) {
            bone.local_transform = bone.bind_pose_transform;
        }
    }


    void SkeletalMesh::loadAnimations(const char* filename, Skeleton& skeleton) {
        Assimp::Importer importer;
        importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

        // aiProcess_LimitBoneWeights; // Limit bone weights to 4 per vertex


        auto path = util::getPath(filename);
        const aiScene* scene = importer.ReadFile(path,SKINNED_IMPORT_PRESET);     // Calculate tangent space for normal mapping

        if (!scene || !scene->mRootNode) {
            debug::error("Failed to load FBX: " + std::string(importer.GetErrorString()));

            return;
        }
        loadAnimations(scene,skeleton);
    }

    void SkeletalMesh::loadAnimations(const aiScene* scene, Skeleton& skeleton) {
        if (!scene->HasAnimations()) return;
        std::unordered_map<std::string, Animation> animations_map;
        for (size_t i = 0; i < scene->mNumAnimations; i++) {
            const aiAnimation* ai_anim = scene->mAnimations[i];


            Animation animation;
            animation.ticks_per_second = ai_anim->mTicksPerSecond != 0.0 ? ai_anim->mTicksPerSecond : 25.0;
            animation.duration = ai_anim->mDuration;
            for (size_t c = 0; c < ai_anim->mNumChannels; c++) {
                const aiNodeAnim* ai_channel = ai_anim->mChannels[c];
                const std::string bone_name = ai_channel->mNodeName.C_Str();
                AnimationChannel channel;

                // Skip channels for bones not in skeleton
                if (!skeleton.bone_map_.contains(bone_name)) {
                    debug::print("Skipping animation channel for unknown bone: " + bone_name);
                    continue;
                }

                channel.bone_id = skeleton.bone_map_.at(bone_name);
                channel.bone_name = bone_name;

                // Position keys
                for (size_t pk = 0; pk < ai_channel->mNumPositionKeys; pk++) {
                    const aiVectorKey& ai_pos_key = ai_channel->mPositionKeys[pk];
                    PositionKey pos_key;
                    pos_key.time = ai_pos_key.mTime;
                    pos_key.value = glm::vec3(ai_pos_key.mValue.x, ai_pos_key.mValue.y, ai_pos_key.mValue.z);
                    channel.position_keys.push_back(pos_key);
                }

                // Rotation keys
                for (size_t rk = 0; rk < ai_channel->mNumRotationKeys; rk++) {
                    const aiQuatKey& ai_rot_key = ai_channel->mRotationKeys[rk];
                    RotationKey rot_key;
                    rot_key.time = ai_rot_key.mTime;
                    rot_key.value = glm::quat(ai_rot_key.mValue.w, ai_rot_key.mValue.x, ai_rot_key.mValue.y, ai_rot_key.mValue.z);
                    channel.rotation_keys.push_back(rot_key);
                }

                // Scale keys
                for (size_t sk = 0; sk < ai_channel->mNumScalingKeys; sk++) {
                    const aiVectorKey& ai_scale_key = ai_channel->mScalingKeys[sk];
                    ScaleKey scale_key;
                    scale_key.time = ai_scale_key.mTime;
                    scale_key.value = glm::vec3(ai_scale_key.mValue.x, ai_scale_key.mValue.y,  ai_scale_key.mValue.z);
                    channel.scale_keys.push_back(scale_key);
                }

                animation.channels[channel.bone_id] = channel;
            }

            animations_map[ai_anim->mName.C_Str()] = animation;
        }

        for (const auto& [name, anim] : animations_map) {
            auto n = name;
            int idx = 0;
            while (skeleton.animations_name_to_index.contains(n)) {
                n = name + "_" + std::to_string(idx++);
            }
            skeleton.animations_name_to_index[n] = skeleton.animations_.size();
            skeleton.animations_.push_back(anim);
        }
    }




    SkinnedMesh SkeletalMesh::loadFbx(const char* filename) {
        auto directory = util::getDirectory(filename);

        Assimp::Importer importer;
        importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

        auto path = util::getPath(filename);
        const aiScene* scene = importer.ReadFile(path,SKINNED_IMPORT_PRESET);     // Calculate tangent space for normal mapping

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            debug::error("Failed to load FBX: " + std::string(importer.GetErrorString()));
            return {};
        }



        auto skeleton = loadSkeleton(scene);
        loadAnimations(scene, skeleton);
        skeleton.updateBoneMatrices();  // Calculate bone matrices for bind pose

        auto materials = Texture::loadSceneMaterials(scene, directory);
        DrawMesh mesh;
        for (size_t i=0; i<scene->mNumMeshes; i++) {
            const aiMesh* aimesh = scene->mMeshes[i];
            if (!aimesh->HasBones()) { continue; }

            for (auto vi =0; vi < aimesh->mNumVertices; vi++) {
                const aiVector3D& pos = aimesh->mVertices[vi];
                skeleton.vertices_.emplace_back(pos.x, pos.y, pos.z);
            }

            for (auto fi = 0; fi < aimesh->mNumFaces; fi++) {
                const aiFace& face = aimesh->mFaces[fi];
                skeleton.faces_.emplace_back(face.mIndices[0], face.mIndices[1], face.mIndices[2]);
            }

            std::vector<glm::vec3> vertices(aimesh->mNumVertices);
            std::vector<BoneIDs> bone_ids(aimesh->mNumVertices, BoneIDs{});
            std::vector<BoneWeights> bone_weights(aimesh->mNumVertices, BoneWeights{});



            for (size_t b = 0; b < aimesh->mNumBones; b++) { // populate bone data
                const aiBone* bone = aimesh->mBones[b];
                auto bone_id = skeleton.bone_map_[bone->mName.C_Str()];



                for (size_t w = 0; w < bone->mNumWeights; w++) {
                    const aiVertexWeight& weight = bone->mWeights[w];
                    const size_t vertex_id = weight.mVertexId;

                    auto& s_bone = skeleton.bones_[bone_id];
                    s_bone.vertex_weights[vertex_id] = weight.mWeight;
                    skeleton.vertex_to_boneID_map_[vertex_id].push_back(bone_id);


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

            // Build unique vertex data (indexed approach)
            std::vector<glm::vec3> positions;
            std::vector<glm::vec3> normals;
            std::vector<glm::vec2> texcoords;
            std::vector<BoneIDs> vertex_bone_ids;
            std::vector<BoneWeights> vertex_bone_weights;

            positions.reserve(aimesh->mNumVertices);
            normals.reserve(aimesh->mNumVertices);
            texcoords.reserve(aimesh->mNumVertices);
            vertex_bone_ids.reserve(aimesh->mNumVertices);
            vertex_bone_weights.reserve(aimesh->mNumVertices);

            // Store unique vertices
            for (unsigned int v = 0; v < aimesh->mNumVertices; v++) {
                const aiVector3D& pos = aimesh->mVertices[v];
                const aiVector3D& normal = aimesh->mNormals[v];
                const aiVector3D& texcoord = aimesh->HasTextureCoords(0) ?
                    aimesh->mTextureCoords[0][v] : aiVector3D(0.0f, 0.0f, 0.0f);

                vertices[v] = glm::vec3(pos.x, pos.y, pos.z);
                positions.push_back(glm::vec3(pos.x, pos.y, pos.z));
                normals.push_back(glm::vec3(normal.x, normal.y, normal.z));
                texcoords.push_back(glm::vec2(texcoord.x, texcoord.y));
                vertex_bone_ids.push_back(bone_ids[v]);
                vertex_bone_weights.push_back(bone_weights[v]);
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
            object.shape = loadSkinnedShapeIndexed(positions, normals, texcoords,
                                                    vertex_bone_ids, vertex_bone_weights, indices);
            object.material = materials[material_name];

            mesh.min = glm::min(mesh.min, object.shape.min);
            mesh.max = glm::max(mesh.max, object.shape.max);
            mesh.objects.push_back(object);

        }



        skeleton.updateBoneMatrices();
        return {mesh, skeleton};
    }

    DrawShape SkeletalMesh::loadSkinnedShapeIndexed(
        const std::vector<glm::vec3>& positions,
        const std::vector<glm::vec3>& normals,
        const std::vector<glm::vec2>& texcoords,
        const std::vector<BoneIDs>& bone_ids,
        const std::vector<BoneWeights>& bone_weights,
        const std::vector<unsigned int>& indices) {

        DrawShape shape;
        GLuint vao, vbo, ebo;

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        // Calculate interleaved buffer size
        const size_t num_vertices = positions.size();
        constexpr size_t floats_per_vertex = 3 + 3 + 2;  // pos + normal + texcoord
        constexpr size_t uints_per_vertex = 4;           // bone IDs
        constexpr size_t weights_per_vertex = 4;         // bone weights

        // Create interleaved VBO: [pos(3), normal(3), texcoord(2), bone_ids(4), bone_weights(4)] per vertex
        std::vector<float> interleaved_data;
        interleaved_data.reserve(num_vertices * (floats_per_vertex + weights_per_vertex));

        std::vector<unsigned int> bone_id_data;
        bone_id_data.reserve(num_vertices * uints_per_vertex);

        for (size_t i = 0; i < num_vertices; i++) {
            // Position
            interleaved_data.push_back(positions[i].x);
            interleaved_data.push_back(positions[i].y);
            interleaved_data.push_back(positions[i].z);

            // Normal
            interleaved_data.push_back(normals[i].x);
            interleaved_data.push_back(normals[i].y);
            interleaved_data.push_back(normals[i].z);

            // Texcoord
            interleaved_data.push_back(texcoords[i].x);
            interleaved_data.push_back(texcoords[i].y);

            // Bone weights (after float data)
            interleaved_data.push_back(bone_weights[i][0]);
            interleaved_data.push_back(bone_weights[i][1]);
            interleaved_data.push_back(bone_weights[i][2]);
            interleaved_data.push_back(bone_weights[i][3]);

            // Bone IDs (stored separately since they're integers)
            bone_id_data.push_back(bone_ids[i][0]);
            bone_id_data.push_back(bone_ids[i][1]);
            bone_id_data.push_back(bone_ids[i][2]);
            bone_id_data.push_back(bone_ids[i][3]);
        }

        const GLsizei stride = (floats_per_vertex + weights_per_vertex) * sizeof(float);

        // Create VBO for float data (pos, normal, texcoord, weights)
        GLuint vbo_float;
        glGenBuffers(1, &vbo_float);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_float);
        glBufferData(GL_ARRAY_BUFFER, interleaved_data.size() * sizeof(float), interleaved_data.data(), GL_STATIC_DRAW);

        // Position (attribute 0)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);

        // Normal (attribute 1)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));

        // Texcoord (attribute 2)
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));

        // Bone weights (attribute 4)
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, stride, (void*)(8 * sizeof(float)));

        // Create separate VBO for bone IDs (integers)
        GLuint vbo_bone_ids;
        glGenBuffers(1, &vbo_bone_ids);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_bone_ids);
        glBufferData(GL_ARRAY_BUFFER, bone_id_data.size() * sizeof(unsigned int), bone_id_data.data(), GL_STATIC_DRAW);

        // Bone IDs (attribute 3) - must use glVertexAttribIPointer for integers
        glEnableVertexAttribArray(3);
        glVertexAttribIPointer(3, 4, GL_UNSIGNED_INT, 0, nullptr);

        // Create and setup EBO
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        // Unbind VAO (preserves EBO binding)
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Calculate bounding box
        glm::vec3 bmin(std::numeric_limits<float>::max());
        glm::vec3 bmax(std::numeric_limits<float>::lowest());
        for (const auto& pos : positions) {
            bmin = glm::min(bmin, pos);
            bmax = glm::max(bmax, pos);
        }

        shape.vao = vao;
        shape.vbo = vbo_float;
        shape.ebo = ebo;
        shape.numTriangles = indices.size() / 3;
        shape.min = bmin;
        shape.max = bmax;

        return shape;
    }

}