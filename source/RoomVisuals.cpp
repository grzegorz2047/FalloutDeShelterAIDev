#include "render/RoomVisuals.hpp"

#include <3ds.h>

#include <algorithm>
#include <cmath>

namespace deep_shelter::render {
namespace {

u32 mix(u32 a, u32 b, float t) {
    const auto ch = [t](u32 lhs, u32 rhs, int shift) {
        const float l = static_cast<float>((lhs >> shift) & 0xffu);
        const float r = static_cast<float>((rhs >> shift) & 0xffu);
        return static_cast<u32>(l + (r - l) * t) & 0xffu;
    };
    return C2D_Color32(ch(a, b, 0), ch(a, b, 8), ch(a, b, 16), ch(a, b, 24));
}

u32 hash_color(int x, int y, u32 dark, u32 light) {
    const unsigned value = static_cast<unsigned>(x * 73856093) ^
                           static_cast<unsigned>(y * 19349663);
    return mix(dark, light, static_cast<float>(value & 7u) / 7.0f);
}

float animation_phase(int room_index) {
    const double seconds = static_cast<double>(osGetTime()) / 1000.0;
    return static_cast<float>(std::fmod(seconds * 0.75 + room_index * 0.137, 1.0));
}

float triangle_wave(float phase) {
    return 1.0f - std::abs(phase * 2.0f - 1.0f);
}

void rect(float x, float y, float z, float w, float h, u32 color, RenderStats& stats) {
    if (w <= 0.0f || h <= 0.0f) return;
    C2D_DrawRectSolid(x, y, z, w, h, color);
    ++stats.draw_calls;
}

void panel_lines(float x, float y, float w, float h, float zoom, RenderStats& stats) {
    const float step = std::max(7.0f, 12.0f * zoom);
    for (float px = x + step; px < x + w; px += step) {
        rect(px, y, 0.14f, std::max(1.0f, zoom), h,
             C2D_Color32(25, 38, 43, 115), stats);
    }
    for (float py = y + step; py < y + h; py += step) {
        rect(x, py, 0.14f, w, std::max(1.0f, zoom),
             C2D_Color32(98, 115, 113, 55), stats);
    }
}

void pipe(float x, float y, float length, float thickness, u32 color, RenderStats& stats) {
    rect(x, y, 0.31f, length, thickness, C2D_Color32(26, 31, 35, 255), stats);
    rect(x, y + thickness * 0.18f, 0.32f, length, thickness * 0.58f, color, stats);
}

void lamp(float x, float y, float scale, float pulse, u32 color, RenderStats& stats) {
    rect(x - 5.0f * scale, y, 0.42f, 10.0f * scale, 2.0f * scale,
         C2D_Color32(40, 45, 47, 255), stats);
    rect(x - 3.0f * scale, y + 2.0f * scale, 0.43f, 6.0f * scale, 2.0f * scale,
         mix(C2D_Color32(76, 79, 73, 180), color, 0.35f + pulse * 0.65f), stats);
}

void console_unit(float x, float floor_y, float scale, u32 accent, float pulse,
                  RenderStats& stats) {
    const float w = 18.0f * scale;
    const float h = 17.0f * scale;
    rect(x, floor_y - h, 0.34f, w, h, C2D_Color32(35, 43, 48, 255), stats);
    rect(x + 2.0f * scale, floor_y - h + 2.0f * scale, 0.35f,
         w - 4.0f * scale, 6.0f * scale, mix(C2D_Color32(37, 65, 67, 255), accent, pulse), stats);
    rect(x + 3.0f * scale, floor_y - 6.0f * scale, 0.35f,
         3.0f * scale, 3.0f * scale, C2D_Color32(234, 188, 72, 255), stats);
    rect(x + w - 6.0f * scale, floor_y - 6.0f * scale, 0.35f,
         3.0f * scale, 3.0f * scale, pulse > 0.48f ? C2D_Color32(95, 212, 148, 255)
                                                    : C2D_Color32(44, 79, 66, 255), stats);
}

void resident(float center_x, float floor_y, float scale, float phase, RenderStats& stats) {
    const float bob = triangle_wave(phase) * 1.2f * scale;
    const float y = floor_y - bob;
    rect(center_x - 3.0f * scale, y - 15.0f * scale, 0.5f,
         6.0f * scale, 7.0f * scale, C2D_Color32(224, 191, 151, 255), stats);
    rect(center_x - 4.0f * scale, y - 8.0f * scale, 0.49f,
         8.0f * scale, 8.0f * scale, C2D_Color32(54, 117, 126, 255), stats);
    rect(center_x - 5.0f * scale, y - 1.5f * scale, 0.48f,
         4.0f * scale, 5.0f * scale, C2D_Color32(28, 34, 39, 255), stats);
    rect(center_x + 1.0f * scale, y - 1.5f * scale, 0.48f,
         4.0f * scale, 5.0f * scale, C2D_Color32(28, 34, 39, 255), stats);
}

void draw_power_equipment(const RoomVisual& v, float ix, float iy, float iw, float ih,
                          float phase, RenderStats& stats) {
    const float s = v.zoom;
    const float pulse = triangle_wave(phase);
    pipe(ix + 4.0f * s, iy + 4.0f * s, iw - 8.0f * s, 4.0f * s,
         mix(C2D_Color32(104, 77, 42, 255), C2D_Color32(238, 168, 61, 255), pulse), stats);
    for (int i = 0; i < 3; ++i) {
        const float local = triangle_wave(std::fmod(phase + i * 0.23f, 1.0f));
        const float x = ix + (7.0f + i * 17.0f) * s;
        rect(x, iy + ih - 22.0f * s, 0.33f, 12.0f * s, 18.0f * s,
             C2D_Color32(52, 64, 69, 255), stats);
        rect(x + 2.0f * s, iy + ih - 19.0f * s, 0.34f, 8.0f * s, 5.0f * s,
             mix(C2D_Color32(93, 70, 39, 255), C2D_Color32(245, 181, 67, 255), local), stats);
    }
}

void draw_hydro_equipment(const RoomVisual& v, float ix, float iy, float iw, float ih,
                          float phase, RenderStats& stats) {
    const float s = v.zoom;
    for (int i = 0; i < 3; ++i) {
        const float sway = (triangle_wave(std::fmod(phase + i * 0.2f, 1.0f)) - 0.5f) * 2.0f * s;
        const float x = ix + (5.0f + i * 18.0f) * s;
        rect(x, iy + ih - 10.0f * s, 0.33f, 14.0f * s, 6.0f * s,
             C2D_Color32(65, 77, 63, 255), stats);
        rect(x + 2.0f * s + sway, iy + ih - 13.0f * s, 0.34f, 3.0f * s, 5.0f * s,
             C2D_Color32(88, 179, 103, 255), stats);
        rect(x + 7.0f * s - sway, iy + ih - 16.0f * s, 0.34f, 3.0f * s, 8.0f * s,
             C2D_Color32(102, 202, 116, 255), stats);
    }
    pipe(ix + 3.0f * s, iy + 5.0f * s, iw - 6.0f * s, 3.0f * s,
         C2D_Color32(74, 156, 167, 255), stats);
}

void draw_water_equipment(const RoomVisual& v, float ix, float iy, float iw, float ih,
                          float phase, RenderStats& stats) {
    const float s = v.zoom;
    const float level = 7.0f + triangle_wave(phase) * 5.0f;
    for (int i = 0; i < 2; ++i) {
        const float x = ix + (8.0f + i * 25.0f) * s;
        rect(x, iy + ih - 24.0f * s, 0.33f, 18.0f * s, 20.0f * s,
             C2D_Color32(41, 67, 76, 255), stats);
        rect(x + 3.0f * s, iy + ih - (7.0f + level) * s, 0.34f,
             12.0f * s, level * s, C2D_Color32(53, 145, 172, 255), stats);
    }
    pipe(ix + 2.0f * s, iy + 4.0f * s, iw - 4.0f * s, 3.0f * s,
         C2D_Color32(56, 145, 183, 255), stats);
}

void draw_workshop_equipment(const RoomVisual& v, float ix, float iy, float iw, float ih,
                             float phase, RenderStats& stats) {
    const float s = v.zoom;
    const float spark = phase > 0.82f ? 1.0f : 0.0f;
    rect(ix + 5.0f * s, iy + ih - 12.0f * s, 0.33f, iw - 10.0f * s, 7.0f * s,
         C2D_Color32(91, 63, 45, 255), stats);
    rect(ix + 12.0f * s, iy + ih - 27.0f * s, 0.34f, 4.0f * s, 15.0f * s,
         C2D_Color32(164, 110, 56, 255), stats);
    rect(ix + 31.0f * s, iy + ih - 24.0f * s, 0.34f, 15.0f * s, 4.0f * s,
         C2D_Color32(181, 128, 66, 255), stats);
    if (spark > 0.0f) {
        rect(ix + 36.0f * s, iy + ih - 30.0f * s, 0.46f, 2.0f * s, 5.0f * s,
             C2D_Color32(255, 222, 116, 255), stats);
    }
    console_unit(ix + iw - 23.0f * s, iy + ih - 4.0f * s, s,
                 C2D_Color32(87, 181, 177, 255), triangle_wave(phase), stats);
}

void draw_storage_equipment(const RoomVisual& v, float ix, float iy, float iw, float ih,
                            float phase, RenderStats& stats) {
    const float s = v.zoom;
    const float crate_width = 14.0f * s;
    const float gap = std::max(2.0f * s, (iw - crate_width * 3.0f) / 4.0f);
    for (int i = 0; i < 3; ++i) {
        const float lift = (i == 1 ? triangle_wave(phase) * 1.5f * s : 0.0f);
        const float x = ix + gap + static_cast<float>(i) * (crate_width + gap);
        rect(x, iy + ih - 18.0f * s - lift, 0.33f, crate_width, 14.0f * s,
             C2D_Color32(84, 70, 55, 255), stats);
        rect(x + 2.0f * s, iy + ih - 16.0f * s - lift, 0.34f, 10.0f * s, 3.0f * s,
             C2D_Color32(189, 143, 73, 255), stats);
    }
}

void draw_living_equipment(const RoomVisual& v, float ix, float iy, float iw, float ih,
                           float phase, RenderStats& stats) {
    const float s = v.zoom;
    const float screen_pulse = triangle_wave(phase);
    rect(ix + 5.0f * s, iy + ih - 11.0f * s, 0.33f, 25.0f * s, 7.0f * s,
         C2D_Color32(73, 87, 88, 255), stats);
    rect(ix + 7.0f * s, iy + ih - 16.0f * s, 0.34f, 21.0f * s, 7.0f * s,
         C2D_Color32(89, 136, 139, 255), stats);
    rect(ix + iw - 18.0f * s, iy + ih - 21.0f * s, 0.33f, 13.0f * s, 17.0f * s,
         C2D_Color32(78, 60, 49, 255), stats);
    rect(ix + iw - 15.0f * s, iy + ih - 18.0f * s, 0.35f, 7.0f * s, 6.0f * s,
         mix(C2D_Color32(39, 62, 65, 255), C2D_Color32(101, 183, 189, 255), screen_pulse), stats);
}

}  // namespace

RoomTheme room_theme_for_index(int room_index) noexcept {
    switch ((room_index % 6 + 6) % 6) {
        case 0: return RoomTheme::Power;
        case 1: return RoomTheme::Hydroponics;
        case 2: return RoomTheme::Water;
        case 3: return RoomTheme::Workshop;
        case 4: return RoomTheme::Storage;
        default: return RoomTheme::Living;
    }
}

void draw_rock_cell(float x,
                    float y,
                    float width,
                    float height,
                    float eye_offset,
                    int column,
                    int row,
                    RenderStats& stats) {
    const u32 base = hash_color(column, row,
                                C2D_Color32(28, 31, 38, 255),
                                C2D_Color32(49, 43, 42, 255));
    rect(x + eye_offset * 0.15f, y, 0.0f, width, height, base, stats);
    const float chip = std::max(2.0f, std::min(width, height) * 0.08f);
    for (int i = 0; i < 4; ++i) {
        const float px = x + std::fmod(static_cast<float>((column * 17 + row * 11 + i * 13) * 7),
                                      std::max(1.0f, width - chip));
        const float py = y + std::fmod(static_cast<float>((column * 5 + row * 19 + i * 9) * 5),
                                      std::max(1.0f, height - chip));
        rect(px + eye_offset * 0.22f, py, 0.01f, chip, chip * 0.55f,
             C2D_Color32(70, 61, 55, 130), stats);
    }
    rect(x + eye_offset * 0.08f, y + height * 0.68f, 0.02f, width, 2.0f,
         C2D_Color32(16, 18, 23, 95), stats);
}

void draw_excavated_cell(float x,
                         float y,
                         float width,
                         float height,
                         float eye_offset,
                         int column,
                         int row,
                         RenderStats& stats) {
    draw_rock_cell(x, y, width, height, eye_offset, column, row, stats);
    rect(x + 2.0f, y + 2.0f, 0.05f, width - 4.0f, height - 4.0f,
         C2D_Color32(24, 43, 48, 255), stats);
    rect(x + 3.0f, y + height - 6.0f, 0.06f, width - 6.0f, 4.0f,
         C2D_Color32(72, 82, 80, 255), stats);
    rect(x + width - 6.0f + eye_offset * 0.18f, y + 5.0f, 0.07f, 3.0f,
         height - 12.0f, C2D_Color32(91, 100, 96, 190), stats);
}

void draw_room_cutaway(const RoomVisual& v, RenderStats& stats) {
    const float s = v.zoom;
    const float phase = animation_phase(v.room_index);
    const float pulse = triangle_wave(phase);
    const float depth = std::max(3.0f, 7.0f * s);
    const float x = v.x;
    const float y = v.y;
    const float w = v.width;
    const float h = v.height;
    const u32 frame = v.selected
                          ? mix(C2D_Color32(151, 110, 42, 255), C2D_Color32(242, 194, 74, 255), pulse)
                          : C2D_Color32(83, 91, 91, 255);

    rect(x - depth + v.eye_offset * 0.45f, y + depth, 0.10f, depth, h - depth,
         C2D_Color32(31, 39, 43, 255), stats);
    rect(x - depth + v.eye_offset * 0.45f, y, 0.10f, w + depth, depth,
         C2D_Color32(51, 61, 64, 255), stats);
    rect(x, y, 0.12f, w, h, C2D_Color32(49, 58, 58, 255), stats);

    const float border = std::max(2.0f, 3.0f * s);
    rect(x, y, 0.20f, w, border, frame, stats);
    rect(x, y + h - border, 0.20f, w, border, frame, stats);
    rect(x, y, 0.20f, border, h, frame, stats);
    rect(x + w - border, y, 0.20f, border, h, frame, stats);

    const float ix = x + border;
    const float iy = y + border;
    const float iw = w - border * 2.0f;
    const float ih = h - border * 2.0f;
    rect(ix, iy, 0.13f, iw, ih, C2D_Color32(53, 66, 68, 255), stats);
    panel_lines(ix, iy, iw, ih, s, stats);
    rect(ix, iy, 0.15f, iw, ih * 0.45f,
         C2D_Color32(16, 20, 22, static_cast<u8>(38 + pulse * 24)), stats);
    rect(ix, iy + ih - std::max(4.0f, 7.0f * s), 0.30f, iw,
         std::max(4.0f, 7.0f * s), C2D_Color32(34, 40, 42, 255), stats);
    lamp(ix + iw * 0.28f, iy + 2.0f * s, s, pulse, C2D_Color32(239, 205, 119, 255), stats);
    lamp(ix + iw * 0.72f, iy + 2.0f * s, s, 1.0f - pulse,
         C2D_Color32(174, 218, 211, 255), stats);

    switch (room_theme_for_index(v.room_index)) {
        case RoomTheme::Power: draw_power_equipment(v, ix, iy, iw, ih, phase, stats); break;
        case RoomTheme::Hydroponics: draw_hydro_equipment(v, ix, iy, iw, ih, phase, stats); break;
        case RoomTheme::Water: draw_water_equipment(v, ix, iy, iw, ih, phase, stats); break;
        case RoomTheme::Workshop: draw_workshop_equipment(v, ix, iy, iw, ih, phase, stats); break;
        case RoomTheme::Storage: draw_storage_equipment(v, ix, iy, iw, ih, phase, stats); break;
        case RoomTheme::Living: draw_living_equipment(v, ix, iy, iw, ih, phase, stats); break;
    }

    const float fill = std::clamp(v.production_fill, 0.0f, 1.0f);
    rect(ix + 3.0f * s, iy + ih - 4.0f * s, 0.45f,
         (iw - 6.0f * s) * fill, std::max(2.0f, 2.5f * s),
         C2D_Color32(89, 211, 138, 255), stats);

    if (v.resident_assigned) {
        resident(ix + iw * 0.55f, iy + ih - 4.0f * s, s, phase, stats);
    }
}

}  // namespace deep_shelter::render
