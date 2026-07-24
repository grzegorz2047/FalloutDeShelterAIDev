#include <3ds.h>
#include <citro2d.h>

#include <algorithm>
#include <cstdio>

#include "render/ShelterCamera.hpp"
#include "ui/UiFramework.hpp"

namespace {

constexpr int kColumns = 12;
constexpr int kRows = 7;
constexpr float kCellWidth = 72.0f;
constexpr float kCellHeight = 52.0f;
constexpr const char* kSavePath = "sdmc:/DeepShelter3D_demo.sav";

struct DemoState {
    int credits = 500;
    int power = 20;
    int food = 20;
    int water = 20;
    int rooms = 1;
    int workers = 0;
    int stored = 0;
    int production_ticks = 0;
    int selected_room = 0;
    bool resident_assigned = false;
    char message[96] = "Zbuduj pokoj, przypisz mieszkanca i odbierz zasoby.";
};

using deep_shelter::render::RenderStats;
using deep_shelter::render::ShelterCamera;
using deep_shelter::ui::InputFrame;
using deep_shelter::ui::UiActionType;
using deep_shelter::ui::UiTree;

void set_message(DemoState& state, const char* message) {
    std::snprintf(state.message, sizeof(state.message), "%s", message);
}

bool save_demo(const DemoState& state) {
    FILE* file = std::fopen(kSavePath, "wb");
    if (file == nullptr) return false;
    const bool ok = std::fwrite(&state, sizeof(state), 1, file) == 1;
    std::fclose(file);
    return ok;
}

bool load_demo(DemoState& state) {
    FILE* file = std::fopen(kSavePath, "rb");
    if (file == nullptr) return false;
    DemoState restored{};
    const bool ok = std::fread(&restored, sizeof(restored), 1, file) == 1;
    std::fclose(file);
    if (!ok || restored.rooms < 1 || restored.rooms > 6 || restored.credits < 0) return false;
    state = restored;
    set_message(state, "Wczytano zapis demonstracyjny.");
    return true;
}

void build_room(DemoState& state) {
    constexpr int kCost = 100;
    if (state.rooms >= 6) {
        set_message(state, "Brak miejsca. Demo obsluguje maksymalnie 6 pokoi.");
        return;
    }
    if (state.credits < kCost) {
        set_message(state, "Za malo kredytow. Odbieraj produkcje.");
        return;
    }
    state.credits -= kCost;
    ++state.rooms;
    state.selected_room = state.rooms - 1;
    set_message(state, "Zbudowano pokoj. Przypisz mieszkanca przyciskiem X.");
}

void assign_resident(DemoState& state) {
    state.resident_assigned = !state.resident_assigned;
    state.workers = state.resident_assigned ? 1 : 0;
    set_message(state, state.resident_assigned
                           ? "Mieszkaniec pracuje. Poczekaj na produkcje."
                           : "Mieszkaniec zostal odwolany z pracy.");
}

void collect(DemoState& state) {
    if (state.stored <= 0) {
        set_message(state, "Brak gotowej produkcji.");
        return;
    }
    const int amount = state.stored;
    state.stored = 0;
    state.power = std::min(100, state.power + amount);
    state.food = std::min(100, state.food + amount / 2);
    state.water = std::min(100, state.water + amount / 2);
    state.credits += amount * 3;
    set_message(state, "Odebrano zasoby i kredyty.");
}

void update_simulation(DemoState& state) {
    if (!state.resident_assigned) return;
    if (++state.production_ticks >= 120) {
        state.production_ticks = 0;
        state.stored = std::min(30, state.stored + 5);
    }
}

void draw_text(C2D_TextBuf buffer, const char* value, float x, float y, float scale, u32 color) {
    C2D_Text text;
    C2D_TextParse(&text, buffer, value);
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, C2D_WithColor, x, y, 0.5f, scale, scale, color);
}

void draw_shelter(C3D_RenderTarget* target,
                  const ShelterCamera& camera,
                  float eye_offset,
                  const DemoState& state,
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
            const bool excavated = row >= 2 && column >= 1 && column <= 10;
            const int room_index = column - 2;
            const bool room = row == 4 && room_index >= 0 && room_index < state.rooms;
            const bool selected = room && room_index == state.selected_room;
            const u32 color = selected
                                  ? C2D_Color32(230, 176, 62, 255)
                                  : room ? C2D_Color32(160, 104, 48, 255)
                                         : excavated ? C2D_Color32(42, 70, 76, 255)
                                                     : C2D_Color32(24, 34, 48, 255);
            C2D_DrawRectSolid(screen_x, screen_y, 0.0f, width, height, color);
            ++stats.draw_calls;
            ++stats.visible_cells;

            if (!room) continue;
            const float fill = selected ? static_cast<float>(state.stored) / 30.0f : 0.0f;
            C2D_DrawRectSolid(screen_x + 4.0f,
                              screen_y + height - 8.0f,
                              0.1f,
                              std::max(0.0f, (width - 8.0f) * fill),
                              4.0f,
                              C2D_Color32(99, 205, 135, 255));
            if (state.resident_assigned && selected) {
                const float size = std::max(4.0f, 9.0f * zoom);
                C2D_DrawRectSolid(screen_x + width * 0.5f - size * 0.5f,
                                  screen_y + height - size - 10.0f,
                                  0.2f,
                                  size,
                                  size,
                                  C2D_Color32(235, 226, 178, 255));
            }
        }
    }
    stats.estimated_linear_memory = stats.visible_cells * sizeof(float) * 8;
}

void draw_button(C2D_TextBuf buffer,
                 float x,
                 int id,
                 int focused_id,
                 bool enabled,
                 const char* label) {
    const bool focused = id == focused_id;
    const u32 color = !enabled ? C2D_Color32(70, 70, 70, 255)
                               : focused ? C2D_Color32(232, 177, 67, 255)
                                         : C2D_Color32(55, 104, 111, 255);
    C2D_DrawRectSolid(x, 160.0f, 0.1f, 66.0f, 34.0f, color);
    draw_text(buffer, label, x + 5.0f, 169.0f, 0.42f, C2D_Color32(255, 255, 255, 255));
}

void draw_bottom(C3D_RenderTarget* bottom,
                 C2D_TextBuf buffer,
                 const DemoState& state,
                 const UiTree& ui) {
    C2D_TargetClear(bottom, C2D_Color32(15, 30, 39, 255));
    C2D_SceneBegin(bottom);

    char status[160];
    std::snprintf(status,
                  sizeof(status),
                  "Kredyty %d   Energia %d   Jedzenie %d   Woda %d",
                  state.credits,
                  state.power,
                  state.food,
                  state.water);
    draw_text(buffer, "DEEP SHELTER 3D - GRYWALNE DEMO", 12.0f, 10.0f, 0.52f,
              C2D_Color32(246, 211, 111, 255));
    draw_text(buffer, status, 12.0f, 34.0f, 0.43f, C2D_Color32(230, 238, 240, 255));

    char room[128];
    std::snprintf(room,
                  sizeof(room),
                  "Pokoje %d/6   Pracownicy %d   Magazyn %d/30",
                  state.rooms,
                  state.workers,
                  state.stored);
    draw_text(buffer, room, 12.0f, 56.0f, 0.43f, C2D_Color32(159, 222, 184, 255));
    draw_text(buffer, state.message, 12.0f, 84.0f, 0.39f, C2D_Color32(255, 255, 255, 255));
    draw_text(buffer,
              "A: buduj  X: przydziel  Y: odbierz  B: zapisz  SELECT: wczytaj",
              12.0f,
              118.0f,
              0.34f,
              C2D_Color32(183, 202, 216, 255));

    const int focused_id = ui.focused_id().value_or(-1);
    draw_button(buffer, 12.0f, 1, focused_id, true, "BUDUJ");
    draw_button(buffer, 88.0f, 2, focused_id, true, "PRACA");
    draw_button(buffer, 164.0f, 3, focused_id, state.stored > 0, "ODBIERZ");
    draw_button(buffer, 240.0f, 4, focused_id, true, "ZAPIS");
}

InputFrame read_ui_input(u32 down, u32 held, u32 up) {
    InputFrame input;
    input.left = (down & KEY_DLEFT) != 0;
    input.right = (down & KEY_DRIGHT) != 0;
    input.confirm = (down & KEY_A) != 0;
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

    if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)) {
        gfxExit();
        return 1;
    }
    if (!C2D_Init(C2D_DEFAULT_MAX_OBJECTS)) {
        C3D_Fini();
        gfxExit();
        return 2;
    }

    C2D_Prepare();
    C3D_RenderTarget* top_left = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* top_right = C2D_CreateScreenTarget(GFX_TOP, GFX_RIGHT);
    C3D_RenderTarget* bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    C2D_TextBuf text_buffer = C2D_TextBufNew(4096);
    if (top_left == nullptr || top_right == nullptr || bottom == nullptr || text_buffer == nullptr) {
        if (text_buffer != nullptr) C2D_TextBufDelete(text_buffer);
        C2D_Fini();
        C3D_Fini();
        gfxExit();
        return 3;
    }

    ShelterCamera camera({kColumns * kCellWidth, kRows * kCellHeight}, {400.0f, 240.0f});
    UiTree ui;
    ui.add({1, {12, 160, 66, 34}, true, true, true, {}, {}});
    ui.add({2, {88, 160, 66, 34}, true, true, true, {}, {}});
    ui.add({3, {164, 160, 66, 34}, true, false, true,
            "Brak gotowej produkcji.", "Przypisz mieszkanca i poczekaj."});
    ui.add({4, {240, 160, 66, 34}, true, true, true, {}, {}});

    DemoState state;
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

        update_simulation(state);
        ui.set_enabled(3,
                       state.stored > 0,
                       "Brak gotowej produkcji.",
                       "Przypisz mieszkanca i poczekaj.");

        if (down & KEY_A) build_room(state);
        if (down & KEY_X) assign_resident(state);
        if (down & KEY_Y) collect(state);
        if (down & KEY_B) {
            set_message(state, save_demo(state) ? "Zapisano demo na karcie SD."
                                                : "Nie udalo sie zapisac gry.");
        }
        if ((down & KEY_SELECT) && !load_demo(state)) {
            set_message(state, "Brak poprawnego zapisu demo.");
        }
        if (down & KEY_DLEFT) state.selected_room = std::max(0, state.selected_room - 1);
        if (down & KEY_DRIGHT) state.selected_room = std::min(state.rooms - 1, state.selected_room + 1);

        const auto action = ui.route(read_ui_input(down, held, up));
        if (action && action->type == UiActionType::Activate) {
            if (action->control_id == 1) build_room(state);
            if (action->control_id == 2) assign_resident(state);
            if (action->control_id == 3) collect(state);
            if (action->control_id == 4) {
                set_message(state, save_demo(state) ? "Zapisano demo na karcie SD."
                                                    : "Nie udalo sie zapisac gry.");
            }
        } else if (action && action->type == UiActionType::ShowDisabledReason) {
            set_message(state, action->message.c_str());
        }

        const float parallax = osGet3DSliderState() * 3.0f;
        RenderStats left_stats{};
        RenderStats right_stats{};
        C2D_TextBufClear(text_buffer);

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        draw_shelter(top_left, camera, -parallax, state, left_stats);
        draw_shelter(top_right, camera, parallax, state, right_stats);
        draw_bottom(bottom, text_buffer, state, ui);
        C3D_FrameEnd(0);
    }

    C2D_TextBufDelete(text_buffer);
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
