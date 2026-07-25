#pragma once

#include <cstddef>

#include <3ds.h>
#include <citro3d.h>

#include "render/Scene3D.hpp"
#include "render/ShelterCamera.hpp"

namespace deep_shelter::render {

// Citro2D's convenience target uses a 16-bit depth buffer for every screen.
// The shelter scene benefits from the PICA200's native 24-bit depth and
// 8-bit stencil support, while the flat lower-screen UI does not need it.
[[nodiscard]] inline C3D_RenderTarget* create_screen_target(
    gfxScreen_t screen,
    gfx3dSide_t side) noexcept {
    const int height = screen == GFX_TOP
                           ? (gfxIsWide() ? GSP_SCREEN_HEIGHT_TOP_2X
                                          : GSP_SCREEN_HEIGHT_TOP)
                           : GSP_SCREEN_HEIGHT_BOTTOM;
    const GPU_DEPTHBUF depth_format =
        screen == GFX_TOP ? GPU_RB_DEPTH24_STENCIL8 : GPU_RB_DEPTH16;

    C3D_RenderTarget* target = C3D_RenderTargetCreate(
        GSP_SCREEN_WIDTH,
        height,
        GPU_RB_RGBA8,
        depth_format);
    if (target == nullptr) return nullptr;

    C3D_RenderTargetSetOutput(
        target,
        screen,
        side,
        GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) |
            GX_TRANSFER_RAW_COPY(0) |
            GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) |
            GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) |
            GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO));
    return target;
}

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
                     bool active,
                     bool selected,
                     bool resident,
                     int stored) noexcept;

    SceneMesh3D mesh_{};
    shaderProgram_s program_{};
    DVLB_s* shader_dvlb_ = nullptr;
    Vertex3D* vertex_buffer_ = nullptr;
    C3D_Tex material_texture_{};
    std::size_t structure_vertex_end_ = 0;
    std::size_t prop_vertex_end_ = 0;
    int projection_uniform_ = -1;
    int model_view_uniform_ = -1;
    bool program_initialized_ = false;
    bool texture_initialized_ = false;
    bool initialized_ = false;
};

}  // namespace deep_shelter::render

// main.cpp includes this header after citro2d.h. Redirect only subsequent calls
// to the project-owned target factory without changing Citro2D itself.
#define C2D_CreateScreenTarget(screen, side) \
    ::deep_shelter::render::create_screen_target((screen), (side))
