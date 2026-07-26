#include <3ds.h>
#include <citro2d.h>

#include <algorithm>
#include <cstdio>

#include "assets/GeneratedUiAtlas.hpp"
#include "render/Scene3DRenderer.hpp"
#include "render/ShelterCamera.hpp"
#include "ui/GeneratedUiRenderer.hpp"
#include "ui/UiFramework.hpp"

namespace {

constexpr int kColumns = 12;
constexpr int kRows = 7;
constexpr float kCellWidth = 72.0f;
constexpr float kCellHeight = 52.0f;
constexpr const char* kSavePath = "sdmc:/DeepShelter3D_demo.sav";
constexpr u32 kRendererDiagnosticColor = 0x9A2020FF;

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

using deep_shelter::assets::UiButtonState;
using deep_shelter::assets::UiIcon;
using deep_shelter::render::RenderStats;
using deep_shelter::render::Scene3DRenderer;
using deep_shelter::render::ShelterCamera;
using deep_shelter::render::ShelterSceneState3D;
using deep_shelter::ui::GeneratedUiRenderer;
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
    ++state.production_ticks;
    if (state.production_ticks >= 120) {
        state.production_ticks = 0;
        state.stored = std::min(30, state.stored + 5);
    }
}

void draw_text(C2D_TextBuf buffer,
               const char* value,
               float x,
               float y,
               float scale,
               u32 color) {
    C2D_Text text;
    C2D_TextParse(&text, buffer, value);
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, C2D_WithColor, x, y, 0.5f, scale, scale, color);
}

const char* room_label(int room_index) {
    switch ((room_index % 6 + 6) % 6) {
        case 0: return "ELEKTROWNIA";
        case 1: return "HYDROPONIKA";
        case 2: return "UZDATNIANIE WODY";
        case 3: return "WARSZTAT";
        case 4: return "MAGAZYN";
        default: return "KWATERY";
    }
}

void draw_resource(GeneratedUiRenderer& atlas,
                   C2D_TextBuf buffer,
                   UiIcon icon,
                   int value,
                   float x,
                   const char* label,
                   u32 accent) {
    char value_text[16];
    std::snprintf(value_text, sizeof(value_text), "%d", value);
    C2D_DrawRectSolid(x, 31.0f, 0.20f, 69.0f, 38.0f,
                      C2D_Color32(24, 38, 45, 255));
    C2D_DrawRectSolid(x, 31.0f, 0.21f, 3.0f, 38.0f, accent);
    atlas.draw_icon(icon, x + 8.0f, 39.0f, 18.0f, 18.0f, 0.35f);
    draw_text(buffer, label, x + 31.0f, 37.0f, 0.28f,
              C2D_Color32(137, 155, 163, 255));
    draw_text(buffer, value_text, x + 31.0f, 50.0f, 0.46f,
              C2D_Color32(238, 235, 216, 255));
}

void draw_button(GeneratedUiRenderer& atlas,
                 C2D_TextBuf buffer,
                 float x,
                 int id,
                 int focused_id,
                 int pressed_id,
                 bool enabled,
                 UiIcon icon,
                 const char* label) {
    UiButtonState state = UiButtonState::Normal;
    if (!enabled) state = UiButtonState::Disabled;
    else if (id == pressed_id) state = UiButtonState::Pressed;
    else if (id == focused_id) state = UiButtonState::Focused;
    const bool focused = enabled && id == focused_id;
    C2D_DrawRectSolid(x, 184.0f, 0.19f, 70.0f, 42.0f,
                      focused ? C2D_Color32(111, 75, 31, 255)
                              : C2D_Color32(27, 39, 44, 255));
    C2D_DrawRectSolid(x, 184.0f, 0.20f, 70.0f, 3.0f,
                      focused ? C2D_Color32(241, 184, 70, 255)
                              : C2D_Color32(67, 82, 86, 255));
    atlas.draw_button_frame(state, x + 2.0f, 186.0f, 66.0f, 38.0f);
    atlas.draw_icon(icon, x + 8.0f, 195.0f, 18.0f, 18.0f, 0.35f);
    draw_text(buffer, label, x + 30.0f, 197.0f, 0.34f,
              enabled ? C2D_Color32(244, 239, 220, 255)
                      : C2D_Color32(105, 114, 117, 255));
}

void draw_bottom(C3D_RenderTarget* bottom,
                 C2D_TextBuf buffer,
                 const DemoState& state,
                 const UiTree& ui,
                 GeneratedUiRenderer& atlas) {
    C2D_Prepare();
    C2D_TargetClear(bottom, C2D_Color32(9, 17, 21, 255));
    C2D_SceneBegin(bottom);
    C2D_DrawRectSolid(0.0f, 0.0f, 0.10f, 320.0f, 25.0f,
                      C2D_Color32(20, 31, 36, 255));
    C2D_DrawRectSolid(0.0f, 24.0f, 0.11f, 320.0f, 2.0f,
                      C2D_Color32(193, 139, 48, 255));
    draw_text(buffer, "DEEP SHELTER", 12.0f, 7.0f, 0.47f,
              C2D_Color32(244, 204, 105, 255));
    draw_text(buffer, "SEKTOR 01  //  ONLINE", 174.0f, 9.0f, 0.28f,
              C2D_Color32(120, 192, 151, 255));
    draw_resource(atlas, buffer, UiIcon::Credits, state.credits, 10.0f,
                  "KREDYTY", C2D_Color32(221, 171, 73, 255));
    draw_resource(atlas, buffer, UiIcon::Power, state.power, 87.0f,
                  "ENERGIA", C2D_Color32(232, 151, 56, 255));
    draw_resource(atlas, buffer, UiIcon::Food, state.food, 164.0f,
                  "ZYWNOSC", C2D_Color32(95, 172, 103, 255));
    draw_resource(atlas, buffer, UiIcon::Water, state.water, 241.0f,
                  "WODA", C2D_Color32(75, 151, 188, 255));
    C2D_DrawRectSolid(10.0f, 77.0f, 0.15f, 300.0f, 82.0f,
                      C2D_Color32(18, 29, 34, 255));
    C2D_DrawRectSolid(10.0f, 77.0f, 0.16f, 4.0f, 82.0f,
                      C2D_Color32(180, 128, 48, 255));
    draw_text(buffer, "ZAZNACZONY POKOJ", 22.0f, 84.0f, 0.27f,
              C2D_Color32(151, 168, 171, 255));
    draw_text(buffer, room_label(state.selected_room), 22.0f, 98.0f, 0.44f,
              C2D_Color32(246, 193, 82, 255));
    draw_text(buffer, state.message, 22.0f, 118.0f, 0.34f,
              C2D_Color32(244, 239, 220, 255));
    char status[128];
    std::snprintf(status, sizeof(status),
                  "POKOJE %d/6  ZALOGA %d  ZAPAS %d/30",
                  state.rooms, state.workers, state.stored);
    draw_text(buffer, status, 22.0f, 140.0f, 0.31f,
              C2D_Color32(113, 196, 151, 255));
    draw_text(buffer, "D-Pad: pokoj  Pad: kamera  L/R: zoom",
              22.0f, 154.0f, 0.29f,
              C2D_Color32(136, 154, 160, 255));
    const int focused_id = ui.focused_id().value_or(-1);
    const int pressed_id = ui.pressed_id().value_or(-1);
    draw_button(atlas, buffer, 10.0f, 1, focused_id, pressed_id, true,
                UiIcon::Build, "BUDUJ");
    draw_button(atlas, buffer, 87.0f, 2, focused_id, pressed_id, true,
                UiIcon::Work, "PRACA");
    draw_button(atlas, buffer, 164.0f, 3, focused_id, pressed_id,
                state.stored > 0, UiIcon::Collect, "ODBIERZ");
    draw_button(atlas, buffer, 241.0f, 4, focused_id, pressed_id, true,
                UiIcon::Save, "ZAPIS");
    C2D_Flush();
}

InputFrame read_ui_input(u32 down, u32 held, u32 up) {
    InputFrame input;
    input.left = (down & KEY_DLEFT) != 0;
    input.right = (down & KEY_DRIGHT) != 0;
    input.confirm = (down & KEY_A) != 0;
    input.cancel = false;
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

    // Both renderers own large fixed buffers. Keep them in static storage rather
    // than using the small 3DS main-thread stack.
    static Scene3DRenderer scene_renderer;
    static GeneratedUiRenderer ui_renderer;
    const bool renderer_ready = scene_renderer.initialize();
    const bool ui_atlas_ready = ui_renderer.initialize();

    ShelterCamera camera({kColumns * kCellWidth, kRows * kCellHeight}, {400.0f, 240.0f});
    UiTree ui;
    ui.add({1, {12, 160, 66, 34}, true, true, true, {}, {}});
    ui.add({2, {88, 160, 66, 34}, true, true, true, {}, {}});
    ui.add({3, {164, 160, 66, 34}, true, false, true,
            "Brak gotowej produkcji.", "Przypisz mieszkanca i poczekaj."});
    ui.add({4, {240, 160, 66, 34}, true, true, true, {}, {}});

    DemoState state;
    if (!renderer_ready) {
        set_message(state, "Blad renderera 3D. Uruchomiono bezpieczny tryb diagnostyczny.");
    } else if (!ui_atlas_ready) {
        set_message(state, "Blad atlasu UI. Uruchomiono interfejs awaryjny.");
    }

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

        // X/Y/B remain direct shortcuts. A activates the focused UI control once.
        if (down & KEY_X) assign_resident(state);
        if (down & KEY_Y) collect(state);
        if (down & KEY_B) {
            set_message(state, save_demo(state) ? "Zapisano demo na karcie SD."
                                                : "Nie udalo sie zapisac gry.");
        }
        if (down & KEY_SELECT) {
            if (!load_demo(state)) set_message(state, "Brak poprawnego zapisu demo.");
        }
        if (down & KEY_DLEFT) state.selected_room = std::max(0, state.selected_room - 1);
        if (down & KEY_DRIGHT)
            state.selected_room = std::min(state.rooms - 1, state.selected_room + 1);

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

        const float stereo = osGet3DSliderState() * 0.035f;
        const ShelterSceneState3D scene_state{state.rooms,
                                               state.selected_room,
                                               state.stored,
                                               state.resident_assigned};
        RenderStats left_stats{};
        RenderStats right_stats{};
        C2D_TextBufClear(text_buffer);

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        if (ui_atlas_ready) {
            draw_bottom(bottom, text_buffer, state, ui, ui_renderer);
        } else {
            C2D_Prepare();
            C2D_TargetClear(bottom, C2D_Color32(15, 30, 39, 255));
            C2D_SceneBegin(bottom);
            draw_text(text_buffer, state.message, 12.0f, 84.0f, 0.39f,
                      C2D_Color32(255, 255, 255, 255));
            C2D_Flush();
        }
        if (renderer_ready) {
            scene_renderer.draw(top_left, camera, -stereo, scene_state, left_stats);
            scene_renderer.draw(top_right, camera, stereo, scene_state, right_stats);
        } else {
            C3D_RenderTargetClear(top_left, C3D_CLEAR_COLOR, kRendererDiagnosticColor, 0);
            C3D_FrameDrawOn(top_left);
            C3D_RenderTargetClear(top_right, C3D_CLEAR_COLOR, kRendererDiagnosticColor, 0);
            C3D_FrameDrawOn(top_right);
        }
        C3D_FrameEnd(0);
    }

    ui_renderer.shutdown();
    scene_renderer.shutdown();
    C2D_TextBufDelete(text_buffer);
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
