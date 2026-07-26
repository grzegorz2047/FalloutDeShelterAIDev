#include "render/ShelterCamera.hpp"

#include <algorithm>

namespace deep_shelter::render {
namespace {

float clamp_axis(float position,
                 float world_size,
                 float viewport_size,
                 float zoom) noexcept {
    const float view_size = viewport_size / zoom;
    if (view_size >= world_size) {
        return (world_size - view_size) * 0.5f;
    }
    return std::clamp(position, 0.0f, world_size - view_size);
}

}  // namespace

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

void ShelterCamera::center_on(float world_x, float world_y) noexcept {
    x_ = world_x - viewport_.width / (2.0f * zoom_);
    y_ = world_y - viewport_.height / (2.0f * zoom_);
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
    x_ = clamp_axis(x_, world_.width, viewport_.width, zoom_);
    y_ = clamp_axis(y_, world_.height, viewport_.height, zoom_);
}

}  // namespace deep_shelter::render
