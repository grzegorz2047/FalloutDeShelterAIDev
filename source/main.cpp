#include <3ds.h>
#include <citro2d.h>

#include <algorithm>
#include <cstdio>

#include "render/ShelterCamera.hpp"
#include "ui/UiFramework.hpp"

namespace {

constexpr int kColumns = 16;
constexpr int kRows = 9;
constexpr float kCellWidth = 72.0f;
constexpr float kCellHeight = 52.0f;

using deep_shelter::render::RenderStats;
using deep_shelter::render::ShelterCamera;
using deep_shelter::ui::InputFrame;
using deep_shelter::ui::UiActionType;
using deep_shelter::ui::UiTree;

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

void draw_button(float x, int id, int focused_id, bool enabled) {
    const bool focused = id == focused_id;
    const u32 color = !enabled ? C2D_Color32(70, 70, 70, 255)
                               : focused ? C2D_Color32(232, 177, 67, 255)
                                         : C2D_Color32(55, 104, 111, 255);
    C2D_DrawRectSolid(x, 154.0f, 0.1f, 58.0f, 24.0f, color);
    if (focused) {
        C2D_DrawRectSolid(x + 4.0f, 182.0f, 0.1f, 50.0f, 3.0f,
                          C2D_Color32(245, 220, 132, 255));
    }
}

void draw_bottom(C3D_RenderTarget* bottom,
                 const ShelterCamera& camera,
                 const RenderStats& stats,
                 const UiTree& ui) {
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
    const int focused_id = ui.focused_id().value_or(-1);
    for (int id = 1; id <= 4; ++id) {
        const auto item = ui.control(id);
        draw_button(24.0f + static_cast<float>(id - 1) * 68.0f,
                    id,
                    focused_id,
                    item && item->enabled);
    }
}

InputFrame read_ui_input(u32 down, u32 held, u32 up) {
    InputFrame input;
    input.up = (down & KEY_DUP) != 0;
    input.down = (down & KEY_DDOWN) != 0;
    input.left = (down & KEY_DLEFT) != 0;
    input.right = (down & KEY_DRIGHT) != 0;
    input.confirm = (down & KEY_A) != 0;
    input.cancel = (down & KEY_B) != 0;
    input.touch_pressed = (down & KEY_TOUCH) != 0;
    input.touch_held = (held & KEY_TOUCH) != 0 && !input.touch_pressed;
    input.touch_released = (up & KEY_TOUCH) != 0;
    if (input.touch_pressed || input.touch_held || input.touch_released) {
        touchPosition touch{};
        hidTouchRead(&touch);
        input.touch_x = touch.px;
        input.touch_y = touch.py;
    }
    return input;
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
    UiTree ui;
    ui.add({1, {24, 154, 58, 32}, true, true, true, {}, {}});
    ui.add({2, {92, 154, 58, 32}, true, true, true, {}, {}});
    ui.add({3, {160, 154, 58, 32}, true, false, true,
            "Brak wybranego pokoju.", "Wybierz pokoj na gornym ekranie."});
    ui.add({4, {228, 154, 58, 32}, true, true, true, {}, {}});

    while (aptMainLoop()) {
        hidScanInput();
        const u32 down = hidKeysDown();
        const u32 held = hidKeysHeld();
        const u32 up = hidKeysUp();
        if (down & KEY_START) break;

        circlePosition circle{};
        hidCircleRead(&circle);
        camera.pan(static_cast<float>(circle.dx) * 0.08f,
                   static_cast<float>(-circle.dy) * 0.08f);
        if (held & KEY_L) camera.zoom_by(-0.025f);
        if (held & KEY_R) camera.zoom_by(0.025f);

        const auto action = ui.route(read_ui_input(down, held, up));
        if (action && action->type == UiActionType::Activate) {
            if (action->control_id == 1) camera.zoom_by(0.15f);
            if (action->control_id == 2) camera.zoom_by(-0.15f);
            if (action->control_id == 4) camera.pan(-10000.0f, -10000.0f);
        }

        const float slider = osGet3DSliderState();
        const float parallax = slider * 3.0f;
        RenderStats left_stats{};
        RenderStats right_stats{};

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        draw_shelter(top_left, camera, -parallax, left_stats);
        draw_shelter(top_right, camera, parallax, right_stats);
        draw_bottom(bottom, camera, left_stats, ui);
        C3D_FrameEnd(0);
    }

    C2D_Fini();
    gfxExit();
    return 0;
}
