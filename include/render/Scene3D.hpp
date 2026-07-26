#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "assets/GeneratedMaterialAtlas.hpp"
#include "assets/RoomAssetAtlas.hpp"

namespace deep_shelter::render {

struct Vertex3D {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float u = 0.0f;
    float v = 0.0f;
    float nx = 0.0f;
    float ny = 0.0f;
    float nz = 1.0f;
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;
};

struct Box3D {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float depth = 0.0f;
    std::uint32_t color = 0xffffffffu;
    assets::GeneratedMaterial material = assets::GeneratedMaterial::Steel;
};

struct Billboard3D {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    std::uint32_t color = 0xffffffffu;
    assets::RoomAtlasRegion region{};
};

struct FixedSideCamera {
    float center_x = 0.0f;
    float center_y = 0.0f;
    float eye_z = 8.0f;
    float zoom = 1.0f;
    float stereo_separation = 0.0f;
};

class SceneMesh3D {
public:
    // Richer room silhouettes and dedicated foreground/glow passes are allowed
    // to use twice the previous geometry budget. The renderer still owns a
    // fixed-capacity mesh, but visual quality now takes priority over the old
    // 4096-vertex constraint.
    static constexpr std::size_t kMaxVertices = 8192;

    void clear() noexcept;
    bool append_box(const Box3D& box) noexcept;
    bool append_billboard(const Billboard3D& billboard) noexcept;

    [[nodiscard]] const Vertex3D* data() const noexcept { return vertices_.data(); }
    [[nodiscard]] std::size_t vertex_count() const noexcept { return vertex_count_; }
    [[nodiscard]] std::size_t box_count() const noexcept { return box_count_; }
    [[nodiscard]] std::size_t billboard_count() const noexcept {
        return billboard_count_;
    }
    [[nodiscard]] bool overflowed() const noexcept { return overflowed_; }

private:
    std::array<Vertex3D, kMaxVertices> vertices_{};
    std::size_t vertex_count_ = 0;
    std::size_t box_count_ = 0;
    std::size_t billboard_count_ = 0;
    bool overflowed_ = false;
};

}  // namespace deep_shelter::render
