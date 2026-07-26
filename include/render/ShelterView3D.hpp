#pragma once

#include <citro3d.h>

#include "render/ShelterCamera.hpp"

namespace deep_shelter::render {

constexpr float kShelterCameraDistance = 680.0f;
constexpr float kShelterStereoFullSlider = 12.0f;

inline void build_shelter_view_matrices(C3D_Mtx& projection,
                                        C3D_Mtx& view,
                                        const ShelterCamera& camera,
                                        float stereo_eye) noexcept {
    const float zoom = camera.zoom();
    const float center_x = camera.x() + 200.0f / zoom;
    const float center_y = camera.y() + 120.0f / zoom;
    Mtx_PerspStereoTilt(&projection,
                        C3D_AngleFromDegrees(31.0f),
                        C3D_AspectRatioTop,
                        1.0f,
                        1600.0f,
                        stereo_eye,
                        kShelterCameraDistance / zoom,
                        false);
    Mtx_LookAt(&view,
               FVec3_New(center_x + 76.0f / zoom,
                         center_y - 92.0f / zoom,
                         kShelterCameraDistance / zoom),
               FVec3_New(center_x, center_y, -18.0f),
               FVec3_New(0.0f, -1.0f, 0.0f),
               false);
}

}  // namespace deep_shelter::render
