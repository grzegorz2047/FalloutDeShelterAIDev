#include "render/ShelterCamera.hpp"

#include <algorithm>

namespace deep_shelter::render {

ShelterCamera::ShelterCamera(WorldBounds world, CameraViewport viewport)
    : world_(world), viewport_(viewport) {
    clamp();
}

void ShelterCamera::pan(float dx, float dy) noexcept {
    x_ += dx / zoom_;
    y_ += dy / zoom_;
    clamp();
}

void ShelterCamera::zoom_by(float delta) noexcept {
    zoom_ = std::clamp(zoom_ + delta, 0.5f, 2.5f);
    clamp();
}

void ShelterCamera::set_world(WorldBounds world) noexcept {
    world_ = world;
    clamp();
}

float ShelterCamera::x() const noexcept { return x_; }
float ShelterCamera::y() const noexcept { return y_; }
float ShelterCamera::zoom() const noexcept { return zoom_; }

bool ShelterCamera::visible(float left, float top, float width, float height) const noexcept {
    const float view_width = viewport_.width / zoom_;
    const float view_height = viewport_.height / zoom_;
    const float right = left + width;
    const float bottom = top + height;
    return right >= x_ && left <= x_ + view_width && bottom >= y_ && top <= y_ + view_height;
}

void ShelterCamera::clamp() noexcept {
    world_.width = std::max(0.0f, world_.width);
    world_.height = std::max(0.0f, world_.height);
    viewport_.width = std::max(1.0f, viewport_.width);
    viewport_.height = std::max(1.0f, viewport_.height);
    const float max_x = std::max(0.0f, world_.width - viewport_.width / zoom_);
    const float max_y = std::max(0.0f, world_.height - viewport_.height / zoom_);
    x_ = std::clamp(x_, 0.0f, max_x);
    y_ = std::clamp(y_, 0.0f, max_y);
}

}  // namespace deep_shelter::render
