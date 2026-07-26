#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <3ds.h>
#include <citro3d.h>

#include "render/Scene3D.hpp"
#include "render/Scene3DRenderer.hpp"
#include "render/ShelterCamera.hpp"

namespace deep_shelter::render {

class DwellerBillboardRenderer {
public:
    static constexpr std::size_t kMaxDwellers = 12;
    static constexpr std::size_t kVerticesPerDweller = 6;
    static constexpr std::size_t kMaxVertices =
        kMaxDwellers * kVerticesPerDweller;

    DwellerBillboardRenderer() = default;
    ~DwellerBillboardRenderer();

    DwellerBillboardRenderer(const DwellerBillboardRenderer&) = delete;
    DwellerBillboardRenderer& operator=(const DwellerBillboardRenderer&) = delete;

    [[nodiscard]] bool initialize() noexcept;
    void shutdown() noexcept;

    void draw(C3D_RenderTarget* target,
              const ShelterCamera& camera,
              float stereo_eye,
              const ShelterSceneState3D& state,
              RenderStats& stats) noexcept;

private:
    void build(const ShelterCamera& camera,
               const ShelterSceneState3D& state) noexcept;
    bool append_dweller(float x,
                         float y,
                         float z,
                         int room_index,
                         bool working,
                         std::uint32_t simulation_tick,
                         std::uint32_t phase) noexcept;

    std::array<Vertex3D, kMaxVertices> vertices_{};
    std::size_t vertex_count_ = 0;
    shaderProgram_s program_{};
    DVLB_s* shader_dvlb_ = nullptr;
    Vertex3D* vertex_buffer_ = nullptr;
    C3D_Tex texture_{};
    int projection_uniform_ = -1;
    int model_view_uniform_ = -1;
    bool program_initialized_ = false;
    bool texture_initialized_ = false;
    bool initialized_ = false;
};

}  // namespace deep_shelter::render
