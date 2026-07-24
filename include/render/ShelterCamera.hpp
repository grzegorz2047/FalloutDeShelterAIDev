#pragma once

#include <cstddef>

namespace deep_shelter::render {

struct WorldBounds {
    float width = 0.0f;
    float height = 0.0f;
};

struct CameraViewport {
    float width = 400.0f;
    float height = 240.0f;
};

struct RenderStats {
    std::size_t draw_calls = 0;
    std::size_t visible_cells = 0;
    std::size_t culled_cells = 0;
    std::size_t estimated_linear_memory = 0;
};

class ShelterCamera {
public:
    ShelterCamera(WorldBounds world, CameraViewport viewport);

    void pan(float dx, float dy) noexcept;
    void zoom_by(float delta) noexcept;
    void set_world(WorldBounds world) noexcept;

    [[nodiscard]] float x() const noexcept;
    [[nodiscard]] float y() const noexcept;
    [[nodiscard]] float zoom() const noexcept;
    [[nodiscard]] bool visible(float left, float top, float width, float height) const noexcept;

private:
    void clamp() noexcept;

    WorldBounds world_;
    CameraViewport viewport_;
    float x_ = 0.0f;
    float y_ = 0.0f;
    float zoom_ = 1.0f;
};

}  // namespace deep_shelter::render
