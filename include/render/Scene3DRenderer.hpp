#pragma once

#include <3ds.h>
#include <citro3d.h>

#include "render/Scene3D.hpp"
#include "render/ShelterCamera.hpp"

namespace deep_shelter::render {

struct ShelterSceneState3D {
    int rooms = 1;
    int selected_room = 0;
    int stored = 0;
    bool resident_assigned = false;
};

class Scene3DRenderer {
public:
    Scene3DRenderer() = default;
    ~Scene3DRenderer();

    Scene3DRenderer(const Scene3DRenderer&) = delete;
    Scene3DRenderer& operator=(const Scene3DRenderer&) = delete;

    [[nodiscard]] bool initialize() noexcept;
    void shutdown() noexcept;

    void draw(C3D_RenderTarget* target,
              const ShelterCamera& camera,
              float stereo_eye,
              const ShelterSceneState3D& state,
              RenderStats& stats) noexcept;

private:
    void build_scene(const ShelterCamera& camera,
                     const ShelterSceneState3D& state,
                     RenderStats& stats) noexcept;
    void append_room(float x,
                     float y,
                     int room_index,
                     bool selected,
                     bool resident,
                     int stored) noexcept;

    SceneMesh3D mesh_{};
    shaderProgram_s program_{};
    DVLB_s* shader_dvlb_ = nullptr;
    Vertex3D* vertex_buffer_ = nullptr;
    int projection_uniform_ = -1;
    int model_view_uniform_ = -1;
    bool program_initialized_ = false;
    bool initialized_ = false;
};

}  // namespace deep_shelter::render