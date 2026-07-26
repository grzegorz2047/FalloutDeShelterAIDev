#pragma once

#include <array>
#include <cstddef>

#include <3ds.h>
#include <citro3d.h>

#include "render/Scene3D.hpp"
#include "render/Scene3DRenderer.hpp"
#include "render/ShelterCamera.hpp"

namespace deep_shelter::render {

class GlowPassRenderer {
public:
    static constexpr std::size_t kMaxGlowVertices = 192;

    GlowPassRenderer() = default;
    ~GlowPassRenderer();

    GlowPassRenderer(const GlowPassRenderer&) = delete;
    GlowPassRenderer& operator=(const GlowPassRenderer&) = delete;

    [[nodiscard]] bool initialize() noexcept;
    void shutdown() noexcept;

    void draw(C3D_RenderTarget* target,
              const ShelterCamera& camera,
              float stereo_eye,
              const ShelterSceneState3D& state) noexcept;

private:
    void build(const ShelterCamera& camera,
               const ShelterSceneState3D& state) noexcept;
    bool append_quad(float x,
                     float y,
                     float z,
                     float width,
                     float height,
                     u32 color) noexcept;

    std::array<Vertex3D, kMaxGlowVertices> vertices_{};
    std::size_t vertex_count_ = 0;
    shaderProgram_s program_{};
    DVLB_s* shader_dvlb_ = nullptr;
    Vertex3D* vertex_buffer_ = nullptr;
    int projection_uniform_ = -1;
    int model_view_uniform_ = -1;
    bool program_initialized_ = false;
    bool initialized_ = false;
};

static_assert(GlowPassRenderer::kMaxGlowVertices >= 156,
              "Glow pass must fit six room halos and one selection frame");

}  // namespace deep_shelter::render
