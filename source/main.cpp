#include <3ds.h>
#include <citro2d.h>

#include <algorithm>
#include <cstdio>

#include "render/ShelterCamera.hpp"

namespace {

constexpr int kColumns = 16;
constexpr int kRows = 9;
constexpr float kCellWidth = 72.0f;
constexpr float kCellHeight = 52.0f;

using deep_shelter::render::RenderStats;
using deep_shelter::render::ShelterCamera;

void draw_shelter(C3D_RenderTarget* target,
                  const ShelterCamera& camera,
                  float eye_offset,
                  RenderStats& stats) {
    C2D_TargetClear(target, C2D_Color32(10, 18, 28, 255));
    C2D_SceneBegin(target);

    const float zoom = camera.zoom();
    for (int row = 0; row < kRows; ++row) {
        for (int column = 0; column < kColumns; ++column) {
            const float world_x = static_cast<float>(column) * kCellWidth;
            const float world_y = static_cast<float>(row) * kCellHeight;
            if (!camera.visible(world_x, world_y, kCellWidth, kCellHeight)) {
                ++stats.culled_cells;
                continue;
            }

            const float screen_x = (world_x - camera.x()) * zoom + eye_offset;
            const float screen_y = (world_y - camera.y()) * zoom;
            const float width = kCellWidth * zoom - 2.0f;
            const float height = kCellHeight * zoom - 2.0f;
            const bool excavated = row >= 2 && column >= 2 && column <= 12;
            const bool room = excavated && row >= 3 && row <= 6 && column >= 3 && column <= 10;
            const u32 color = room
                                  ? C2D_Color32(190, 132, 58, 255)
                                  : excavated ? C2D_Color32(42, 70, 76, 255)
                                              : C2D_Color32(24, 34, 48, 255);
            C2D_DrawRectSolid(screen_x, screen_y, 0.0f, width, height, color);
            ++stats.draw_calls;
            ++stats.visible_cells;

            if (room && ((row + column) % 3 == 0)) {
                const float resident_size = std::max(3.0f, 7.0f * zoom);
                C2D_DrawRectSolid(screen_x + width * 0.5f - resident_size * 0.5f,
                                  screen_y + height - resident_size - 4.0f,
                                  0.1f,
                                  resident_size,
                                  resident_size,
                                  C2D_Color32(224, 218, 170, 255));
                ++stats.draw_calls;
            }
        }
    }
    stats.estimated_linear_memory = stats.visible_cells * sizeof(float) * 8;
}

void draw_bottom(C3D_RenderTarget* bottom, const ShelterCamera& camera, const RenderStats& stats) {
    C2D_TargetClear(bottom, C2D_Color32(15, 30, 39, 255));
    C2D_SceneBegin(bottom);

    C2D_DrawRectSolid(12.0f, 12.0f, 0.0f, 296.0f, 34.0f, C2D_Color32(31, 59, 67, 255));
    C2D_DrawRectSolid(12.0f, 56.0f, 0.0f, 142.0f, 72.0f, C2D_Color32(44, 81, 83, 255));
    C2D_DrawRectSolid(166.0f, 56.0f, 0.0f, 142.0f, 72.0f, C2D_Color32(44, 81, 83, 255));

    const float zoom_bar = std::clamp((camera.zoom() - 0.5f) / 2.0f, 0.0f, 1.0f) * 280.0f;
    C2D_DrawRectSolid(20.0f, 20.0f, 0.1f, zoom_bar, 8.0f, C2D_Color32(222, 166, 66, 255));

    const float visible_ratio = std::min(1.0f, static_cast<float>(stats.visible_cells) / 60.0f);
    C2D_DrawRectSolid(24.0f, 94.0f, 0.1f, 116.0f * visible_ratio, 10.0f,
                      C2D_Color32(93, 176, 133, 255));
    const float draw_ratio = std::min(1.0f, static_cast<float>(stats.draw_calls) / 80.0f);
    C2D_DrawRectSolid(178.0f, 94.0f, 0.1f, 116.0f * draw_ratio, 10.0f,
                      C2D_Color32(102, 154, 210, 255));

    C2D_DrawRectSolid(12.0f, 140.0f, 0.0f, 296.0f, 84.0f, C2D_Color32(23, 45, 55, 255));
    C2D_DrawRectSolid(24.0f, 154.0f, 0.1f, 58.0f, 24.0f, C2D_Color32(194, 134, 61, 255));
    C2D_DrawRectSolid(92.0f, 154.0f, 0.1f, 58.0f, 24.0f, C2D_Color32(55, 104, 111, 255));
    C2D_DrawRectSolid(160.0f, 154.0f, 0.1f, 58.0f, 24.0f, C2D_Color32(55, 104, 111, 255));
    C2D_DrawRectSolid(228.0f, 154.0f, 0.1f, 58.0f, 24.0f, C2D_Color32(55, 104, 111, 255));
}

}  // namespace

int main() {
    gfxInitDefault();
    gfxSet3D(true);
    consoleInit(GFX_BOTTOM, nullptr);

    if (!C2D_Init(C2D_DEFAULT_MAX_OBJECTS)) {
        std::printf("Deep Shelter 3D\n\nCitro2D initialization failed.\nPress START to exit.\n");
        while (aptMainLoop()) {
            hidScanInput();
            if (hidKeysDown() & KEY_START) break;
            gspWaitForVBlank();
        }
        gfxExit();
        return 1;
    }

    C2D_Prepare();
    C3D_RenderTarget* top_left = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* top_right = C2D_CreateScreenTarget(GFX_TOP, GFX_RIGHT);
    C3D_RenderTarget* bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    if (top_left == nullptr || top_right == nullptr || bottom == nullptr) {
        C2D_Fini();
        gfxExit();
        return 2;
    }

    ShelterCamera camera({kColumns * kCellWidth, kRows * kCellHeight}, {400.0f, 240.0f});

    while (aptMainLoop()) {
        hidScanInput();
        const u32 down = hidKeysDown();
        const u32 held = hidKeysHeld();
        if (down & KEY_START) break;

        circlePosition circle{};
        hidCircleRead(&circle);
        camera.pan(static_cast<float>(circle.dx) * 0.08f,
                   static_cast<float>(-circle.dy) * 0.08f);
        if (held & KEY_L) camera.zoom_by(-0.025f);
        if (held & KEY_R) camera.zoom_by(0.025f);

        const float slider = osGet3DSliderState();
        const float parallax = slider * 3.0f;
        RenderStats left_stats{};
        RenderStats right_stats{};

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        draw_shelter(top_left, camera, -parallax, left_stats);
        draw_shelter(top_right, camera, parallax, right_stats);
        draw_bottom(bottom, camera, left_stats);
        C3D_FrameEnd(0);
    }

    C2D_Fini();
    gfxExit();
    return 0;
}
