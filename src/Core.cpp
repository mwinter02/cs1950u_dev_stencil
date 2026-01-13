#include "Core.h"

#include <iostream>

#include "Debug.h"
#include "Window.h"
#include "render/Camera.h"
#include "render/Mesh.h"
#include "render/SkeletalMesh.h"
#include "render/shapes/Cone.h"
#include "render/shapes/Cube.h"
#include "render/shapes/Cylinder.h"
#include "render/shapes/Quad.h"
#include "render/shapes/Sphere.h"


static gl::SkinnedMesh skinned_mesh;
static gl::Transform skinned_transform;

static gl::DrawMesh obj_mesh;
static gl::Transform obj_transform;
Core::Core() : m_camera(std::make_shared<gl::Camera>()), m_light(std::make_shared<gl::Light>()) {

    m_light->position = glm::vec3(0, 5, 0);

    auto cube = gl::Cube(1);
    auto cone = gl::Cone(8,4);
    auto cylinder = gl::Cylinder(8,1);
    auto sphere = gl::Sphere(16,16);
    auto quad = gl::Quad(1);

    gl::Graphics::addShape("cube", cube.loadDrawShape());
    gl::Graphics::addShape("cone", cone.loadDrawShape());
    gl::Graphics::addShape("cylinder", cylinder.loadDrawShape());
    gl::Graphics::addShape("sphere", sphere.loadDrawShape());
    gl::Graphics::addShape("quad", quad.loadDrawShape());


    auto object = Object("cone");
    object.transform.setPosition(glm::vec3(0, 0, -5));

    auto object2 = Object("cube");
    object2.transform.setPosition(glm::vec3(0, 0, 5));


    auto object3 = Object("cylinder");
    object3.transform.setPosition(glm::vec3(5, 0, 0));

    auto object4 = Object("sphere");
    object4.transform.setPosition(glm::vec3(-5, 0, 0));

    auto object5 = Object("quad");
    object5.transform.setPosition(glm::vec3(0, -0.5, 0));

    m_shapes.push_back(object);
    m_shapes.push_back(object2);
    m_shapes.push_back(object3);
    m_shapes.push_back(object4);
    m_shapes.push_back(object5);

    // auto name = "Resources/Models/sponza/sponza.obj";
    auto name = "Resources/Models/Samples/Spider/spider.obj";
    // auto name = "Resources/Models/Samples/Skull/12140_Skull_v3_L2.obj";

    obj_mesh = gl::Mesh::loadStaticMesh(name);
    obj_transform.setScale(glm::vec3(0.1));

    skinned_mesh = gl::SkeletalMesh::loadFbx("Resources/Models/Samples/walking.fbx");
    skinned_transform.setScale(glm::vec3(0.01f));
    skinned_mesh.skeleton.setCurrentAnimation(0);


}

static bool animation_playing = true;

void Core::draw() const {
    gl::Graphics::usePhongShader();
    gl::Graphics::setCameraUniforms(m_camera.get());
    gl::Graphics::setLight(*m_light);
    for (const auto& obj : m_shapes) {
        gl::Graphics::drawObject(obj.shape, obj.transform, obj.material);
    }
    gl::Graphics::drawMesh(&obj_mesh, obj_transform);

    gl::Graphics::useSkinnedShader();
    gl::Graphics::setCameraUniforms(m_camera.get());
    gl::Graphics::setLight(*m_light);

    gl::Graphics::drawSkinned(&skinned_mesh, skinned_transform);
}

static glm::vec2 rotation(0.0f, 0.0f);
static auto last_mouse_pos = Window::getMousePosition();
void Core::update(double delta_time) {
    controller(delta_time);



    if (!Window::isCursorVisible()) {
        auto mouse_pos = Window::getMousePosition();
        auto d_mouse = 0.1f*(last_mouse_pos - mouse_pos);
        last_mouse_pos = mouse_pos;
        rotation.x += d_mouse.y;
        rotation.y += d_mouse.x;
        rotation.x = glm::clamp(rotation.x, -89.0f, 89.0f);


        glm::vec3 newFront;
        newFront.x = sin(glm::radians(rotation.y)) * cos(glm::radians(rotation.x));
        newFront.y = sin(glm::radians(rotation.x));
        newFront.z = cos(glm::radians(rotation.y)) * cos(glm::radians(rotation.x));
        auto newLook = glm::normalize(newFront);

        m_camera->setLook(newLook);
    }

    if (animation_playing) {
        skinned_mesh.skeleton.playCurrentAnimation(delta_time);
    }
}

void Core::controller(double delta_time) {
    float mod = 4.f*delta_time;

    auto camXZ = [](glm::vec3 look) {
        auto cam_xz = look * glm::vec3(1,0,1);
        cam_xz = glm::normalize(cam_xz);
        return cam_xz;
    };
    if (Window::key(GLFW_KEY_W)) {
        m_camera->setPosition(m_camera->getPosition() + mod * camXZ(m_camera->getLook()));
    }
    if (Window::key(GLFW_KEY_S)) {
        m_camera->setPosition(m_camera->getPosition() - mod * camXZ(m_camera->getLook()));
    }
    if (Window::key(GLFW_KEY_A)) {
        m_camera->setPosition(m_camera->getPosition() - mod * camXZ(m_camera -> getRight()));
    }
    if (Window::key(GLFW_KEY_D)) {
        m_camera->setPosition(m_camera->getPosition() + mod * camXZ(m_camera -> getRight()));
    }
    if (Window::key(GLFW_KEY_SPACE)) {
        m_camera->setPosition(m_camera->getPosition() + mod * m_camera->getUp());
    }
    if (Window::key(GLFW_KEY_LEFT_SHIFT)) {
        m_camera->setPosition(m_camera->getPosition() - mod * m_camera -> getUp());
    }

    if (Window::key(GLFW_KEY_ENTER)) {
        Window::hideMouse();
    }
    if (Window::key(GLFW_KEY_ESCAPE)) {
        Window::showMouse();
    }
}

void Core::keyPressed(int key) {
    switch (key) {
    case GLFW_KEY_P: {
        animation_playing = !animation_playing;
        break;
    }

    }
}

