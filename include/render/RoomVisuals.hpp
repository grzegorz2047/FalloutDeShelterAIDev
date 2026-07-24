#pragma once

#include <citro2d.h>

#include "render/ShelterCamera.hpp"

namespace deep_shelter::render {

enum class RoomTheme {
    Power,
    Hydroponics,
    Water,
    Workshop,
    Storage,
    Living,
};

struct RoomVisual {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float zoom = 1.0f;
    float eye_offset = 0.0f;
    float production_fill = 0.0f;
    int room_index = 0;
    bool selected = false;
    bool resident_assigned = false;
};

[[nodiscard]] RoomTheme room_theme_for_index(int room_index) noexcept;

void draw_rock_cell(float x,
                    float y,
                    float width,
                    float height,
                    float eye_offset,
                    int column,
                    int row,
                    RenderStats& stats);

void draw_excavated_cell(float x,
                         float y,
                         float width,
                         float height,
                         float eye_offset,
                         int column,
                         int row,
                         RenderStats& stats);

void draw_room_cutaway(const RoomVisual& visual, RenderStats& stats);

}  // namespace deep_shelter::render
