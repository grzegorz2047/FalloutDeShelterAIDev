#include <3ds.h>
#include <citro2d.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>

#include "assets/GeneratedUiAtlas.hpp"
#include "core/FixedStepClock.hpp"
#include "gameplay/PlayableShelterSession.hpp"
#include "render/DwellerBillboardRenderer.hpp"
#include "render/GlowPassRenderer.hpp"
#include "render/RoomVisualProfile.hpp"
#include "render/Scene3DRenderer.hpp"
#include "render/ShelterCamera.hpp"
#include "render/ShelterSceneLayout.hpp"
#include "render/ShelterView3D.hpp"
#include "ui/GeneratedUiRenderer.hpp"
#include "ui/ShelterHudLayout.hpp"
#include "ui/UiFramework.hpp"

namespace {

constexpr const char* kSavePath = "sdmc:/DeepShelter3D_demo";
constexpr u32 kRendererDiagnosticColor = 0x9A2020FF;

using deep_shelter::assets::UiButtonState;
using deep_shelter::assets::UiIcon;
using deep_shelter::gameplay::BuildPreviewStatus;
using deep_shelter::gameplay::BuildResult;
using deep_shelter::gameplay::CollectResult;
using deep_shelter::gameplay::PlayableBuildPreview;
using deep_shelter::gameplay::PlayableResidentState;
using deep_shelter::gameplay::PlayableRoomType;
using deep_shelter::gameplay::PlayableSaveStatus;
using deep_shelter::gameplay::PlayableShelterSession;
using deep_shelter::gameplay::PlayableShelterState;
using deep_shelter::gameplay::PrimaryAction;
using deep_shelter::render::DwellerBillboardRenderer;
using deep_shelter::render::GlowPassRenderer;
using deep_shelter::render::RenderStats;
using deep_shelter::render::Scene3DRenderer;
using deep_shelter::render::ShelterCamera;
using deep_shelter::render::ShelterSceneState3D;
using deep_shelter::ui::GeneratedUiRenderer;
using deep_shelter::ui::InputFrame;
using deep_shelter::ui::UiActionType;
using deep_shelter::ui::UiTree;

void set_notice(char* output,
                std::size_t output_size,
                const char* message) {
    std::snprintf(output, output_size, "%s", message);
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

const char* room_type_label(PlayableRoomType type) {
    switch (type) {
        case PlayableRoomType::Power: return "ELEKTROWNIA";
        case PlayableRoomType::Food: return "STOLOWKA";
        case PlayableRoomType::Water: return "UZDATNIANIE";
        case PlayableRoomType::Workshop: return "WARSZTAT";
        case PlayableRoomType::Living: return "KWATERY";
        case PlayableRoomType::Elevator: return "WINDA";
    }
    return "POKOJ";
}

UiIcon room_type_icon(PlayableRoomType type) {
    switch (type) {
        case PlayableRoomType::Power: return UiIcon::Power;
        case PlayableRoomType::Food: return UiIcon::Food;
        case PlayableRoomType::Water: return UiIcon::Water;
        case PlayableRoomType::Workshop: return UiIcon::Work;
        case PlayableRoomType::Living: return UiIcon::Credits;
        case PlayableRoomType::Elevator: return UiIcon::Build;
    }
    return UiIcon::Build;
}

const char* production_label(PlayableRoomType type) {
    switch (type) {
        case PlayableRoomType::Power: return "ENERGIA +5";
        case PlayableRoomType::Food: return "ZYWNOSC +5";
        case PlayableRoomType::Water: return "WODA +5";
        case PlayableRoomType::Workshop: return "KREDYTY";
        case PlayableRoomType::Living: return "POJEMNOSC";
        case PlayableRoomType::Elevator: return "TRANSPORT";
    }
    return "EFEKT";
}

int visual_profile(PlayableRoomType type) {
    switch (type) {
        case PlayableRoomType::Power: return 0;
        case PlayableRoomType::Food: return 1;
        case PlayableRoomType::Water: return 2;
        case PlayableRoomType::Workshop: return 3;
        case PlayableRoomType::Living: return 5;
        case PlayableRoomType::Elevator: return 0;
    }
    return 0;
}

const char* preview_status_label(BuildPreviewStatus status) {
    switch (status) {
        case BuildPreviewStatus::Valid: return "GOTOWE";
        case BuildPreviewStatus::OutOfBounds: return "POZA SIATKA";
        case BuildPreviewStatus::Occupied: return "ZAJETE";
        case BuildPreviewStatus::NotEnoughCredits: return "BRAK KREDYTOW";
        case BuildPreviewStatus::Full: return "BRAK MIEJSCA";
    }
    return "NIEDOSTEPNE";
}

const char* build_result_notice(BuildResult result) {
    switch (result) {
        case BuildResult::Built: return "Zbudowano wybrany modul.";
        case BuildResult::NotEnoughCredits: return "Za malo kredytow.";
        case BuildResult::Full: return "Brak wolnego miejsca.";
        case BuildResult::InvalidPlacement:
            return "Nie mozna budowac w tej komorce.";
    }
    return "Budowa nieudana.";
}

PlayableRoomType adjacent_room_type(PlayableRoomType current, int delta) {
    constexpr int kTypeCount = 6;
    const int value = static_cast<int>(current);
    return static_cast<PlayableRoomType>(
        (value + delta + kTypeCount) % kTypeCount);
}

void center_on_cell(ShelterCamera& camera, int column, int floor) {
    camera.center_on(
        deep_shelter::render::layout::room_x(column) +
            deep_shelter::render::layout::kRoomWidth * 0.5f,
        deep_shelter::render::layout::room_y(floor) +
            deep_shelter::render::layout::kRoomHeight * 0.5f);
}

void center_on_selected(ShelterCamera& camera,
                        const PlayableShelterSession& session) {
    const auto& state = session.state();
    if (state.selected_room < 0 ||
        state.selected_room >= deep_shelter::gameplay::kPlayableRoomCapacity) {
        return;
    }
    const auto& room = state.room_entries[
        static_cast<std::size_t>(state.selected_room)];
    if (room.active) center_on_cell(camera, room.column, room.floor);
}

ShelterSceneState3D make_scene_state(
    const PlayableShelterSession& session,
    bool build_mode,
    std::uint32_t animation_tick) {
    ShelterSceneState3D scene;
    const auto& state = session.state();
    scene.animation_tick = animation_tick;

    for (std::size_t index = 0;
         index < state.room_entries.size() &&
         scene.room_count < scene.rooms.size();
         ++index) {
        const auto& source = state.room_entries[index];
        if (!source.active) continue;
        auto& target = scene.rooms[scene.room_count++];
        target.grid_column = source.column;
        target.grid_floor = source.floor;
        target.visual_profile = visual_profile(source.type);
        target.stored = source.stored;
        target.selected = static_cast<int>(index) == state.selected_room;
        target.elevator = source.type == PlayableRoomType::Elevator;
    }

    for (std::size_t index = 0;
         index < state.residents.size() &&
         scene.resident_count < scene.residents.size();
         ++index) {
        const auto& resident = state.residents[index];
        if (!resident.active) continue;
        const auto position = session.resident_position(index);
        auto& target = scene.residents[scene.resident_count++];
        target.world_x =
            deep_shelter::render::layout::kWorldPaddingX +
            position.column * deep_shelter::render::layout::kRoomPitchX +
            48.0f;
        target.world_y =
            deep_shelter::render::layout::kWorldPaddingY +
            position.floor * deep_shelter::render::layout::kRoomPitchY +
            17.0f;
        target.archetype = 4;
        if (resident.assigned_room >= 0 &&
            resident.assigned_room <
                deep_shelter::gameplay::kPlayableRoomCapacity) {
            const auto& room = state.room_entries[
                static_cast<std::size_t>(resident.assigned_room)];
            if (room.active) target.archetype = visual_profile(room.type);
        }
        target.moving = resident.state != PlayableResidentState::Working;
        target.working = resident.state == PlayableResidentState::Working;
        target.animation_phase =
            animation_tick + static_cast<std::uint32_t>(index * 17u);
    }

    if (build_mode) {
        const PlayableBuildPreview preview = session.preview_build();
        scene.build_preview.active = true;
        scene.build_preview.grid_column = session.build_cursor_column();
        scene.build_preview.grid_floor = session.build_cursor_floor();
        scene.build_preview.visual_profile =
            visual_profile(session.selected_build_type());
        scene.build_preview.elevator =
            session.selected_build_type() == PlayableRoomType::Elevator;
        scene.build_preview.valid = preview.valid();
    }
    return scene;
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
    C2D_DrawRectSolid(x, 4.0f, 0.20f, 75.0f, 31.0f,
                      C2D_Color32(22, 35, 40, 255));
    C2D_DrawRectSolid(x, 4.0f, 0.21f, 3.0f, 31.0f, accent);
    atlas.draw_icon(icon, x + 8.0f, 11.0f, 16.0f, 16.0f, 0.35f);
    draw_text(buffer, label, x + 29.0f, 7.0f, 0.32f,
              C2D_Color32(137, 155, 163, 255));
    draw_text(buffer, value_text, x + 29.0f, 18.0f, 0.48f,
              C2D_Color32(238, 235, 216, 255));
}

void draw_button(GeneratedUiRenderer& atlas,
                 C2D_TextBuf buffer,
                 deep_shelter::ui::Rect bounds,
                 int id,
                 int focused_id,
                 int pressed_id,
                 bool enabled,
                 bool primary,
                 UiIcon icon,
                 const char* label) {
    UiButtonState state = UiButtonState::Normal;
    if (!enabled) state = UiButtonState::Disabled;
    else if (id == pressed_id) state = UiButtonState::Pressed;
    else if (id == focused_id) state = UiButtonState::Focused;
    const bool focused = id == focused_id;
    const float x = static_cast<float>(bounds.x);
    const float y = static_cast<float>(bounds.y);
    const float width = static_cast<float>(bounds.width);
    const float height = static_cast<float>(bounds.height);
    u32 fill = C2D_Color32(25, 38, 43, 255);
    if (!enabled) fill = C2D_Color32(31, 35, 37, 255);
    if (enabled && state == UiButtonState::Pressed) {
        fill = C2D_Color32(126, 83, 31, 255);
    } else if (focused) {
        fill = enabled ? C2D_Color32(90, 64, 30, 255)
                       : C2D_Color32(50, 57, 58, 255);
    }
    C2D_DrawRectSolid(x, y, 0.19f, width, height, fill);
    const u32 border = focused
                           ? C2D_Color32(241, 184, 70, 255)
                           : C2D_Color32(67, 82, 86, 255);
    C2D_DrawRectSolid(x, y, 0.20f, width,
                      primary ? 5.0f : 3.0f, border);
    C2D_DrawRectSolid(x, y + height - 2.0f, 0.20f,
                      width, 2.0f, border);
    C2D_DrawRectSolid(x, y, 0.20f, 2.0f, height, border);
    C2D_DrawRectSolid(x + width - 2.0f, y, 0.20f,
                      2.0f, height, border);
    const float icon_size = primary ? 30.0f : 20.0f;
    atlas.draw_icon(icon,
                    x + (primary ? 15.0f : 9.0f),
                    y + (height - icon_size) * 0.5f,
                    icon_size,
                    icon_size,
                    0.35f);
    if (primary) {
        draw_text(buffer, "AKCJA", x + 57.0f, y + 8.0f, 0.38f,
                  C2D_Color32(168, 181, 181, 255));
    }
    draw_text(buffer,
              label,
              x + (primary ? 57.0f : 35.0f),
              y + (primary ? 33.0f : 8.0f),
              primary ? 0.58f : 0.42f,
              enabled ? C2D_Color32(244, 239, 220, 255)
                      : C2D_Color32(105, 114, 117, 255));
}

const char* primary_label(PrimaryAction action) {
    switch (action) {
        case PrimaryAction::Assign: return "PRZYPISZ";
        case PrimaryAction::Collect: return "ODBIERZ";
        case PrimaryAction::Wait: return "PRODUKCJA";
    }
    return "AKCJA";
}

UiIcon primary_icon(PrimaryAction action) {
    return action == PrimaryAction::Collect ? UiIcon::Collect : UiIcon::Work;
}

void draw_bottom(C3D_RenderTarget* bottom,
                 C2D_TextBuf buffer,
                 const PlayableShelterSession& session,
                 const UiTree& ui,
                 GeneratedUiRenderer& atlas,
                 const char* notice,
                 bool build_mode) {
    using namespace deep_shelter::ui::shelter_hud;
    const auto& state = session.state();

    C2D_Prepare();
    C2D_TargetClear(bottom, C2D_Color32(9, 17, 21, 255));
    C2D_SceneBegin(bottom);
    draw_resource(atlas, buffer, UiIcon::Credits, state.credits, 4.0f,
                  "KREDYTY", C2D_Color32(221, 171, 73, 255));
    draw_resource(atlas, buffer, UiIcon::Power, state.power, 83.0f,
                  "ENERGIA", C2D_Color32(232, 151, 56, 255));
    draw_resource(atlas, buffer, UiIcon::Food, state.food, 162.0f,
                  "ZYWNOSC", C2D_Color32(95, 172, 103, 255));
    draw_resource(atlas, buffer, UiIcon::Water, state.water, 241.0f,
                  "WODA", C2D_Color32(75, 151, 188, 255));

    C2D_DrawRectSolid(8.0f, 40.0f, 0.15f, 304.0f, 112.0f,
                      C2D_Color32(18, 29, 34, 255));
    C2D_DrawRectSolid(8.0f, 40.0f, 0.16f, 4.0f, 112.0f,
                      build_mode ? C2D_Color32(79, 196, 145, 255)
                                 : C2D_Color32(180, 128, 48, 255));

    if (build_mode) {
        const PlayableRoomType type = session.selected_build_type();
        const PlayableBuildPreview preview = session.preview_build();
        atlas.draw_icon(room_type_icon(type),
                        20.0f, 48.0f, 22.0f, 22.0f, 0.35f);
        draw_text(buffer, "TRYB BUDOWY", 50.0f, 44.0f, 0.36f,
                  C2D_Color32(151, 168, 171, 255));
        draw_text(buffer, room_type_label(type), 50.0f, 57.0f, 0.50f,
                  preview.valid() ? C2D_Color32(112, 225, 164, 255)
                                  : C2D_Color32(246, 126, 99, 255));

        C2D_DrawRectSolid(16.0f, 78.0f, 0.18f, 88.0f, 35.0f,
                          C2D_Color32(24, 39, 44, 255));
        C2D_DrawRectSolid(112.0f, 78.0f, 0.18f, 88.0f, 35.0f,
                          C2D_Color32(24, 39, 44, 255));
        C2D_DrawRectSolid(208.0f, 78.0f, 0.18f, 88.0f, 35.0f,
                          C2D_Color32(24, 39, 44, 255));
        draw_text(buffer, "KOSZT", 22.0f, 81.0f, 0.34f,
                  C2D_Color32(137, 155, 163, 255));
        char cost_text[24];
        std::snprintf(cost_text, sizeof(cost_text), "%d KR", preview.cost);
        draw_text(buffer, cost_text, 22.0f, 96.0f, 0.42f,
                  C2D_Color32(238, 198, 99, 255));
        draw_text(buffer, "POZYCJA", 118.0f, 81.0f, 0.34f,
                  C2D_Color32(137, 155, 163, 255));
        char position_text[32];
        std::snprintf(position_text,
                      sizeof(position_text),
                      "C%d F%d",
                      session.build_cursor_column() + 1,
                      session.build_cursor_floor() + 1);
        draw_text(buffer, position_text, 118.0f, 96.0f, 0.42f,
                  C2D_Color32(112, 207, 159, 255));
        draw_text(buffer, "STATUS", 214.0f, 81.0f, 0.34f,
                  C2D_Color32(137, 155, 163, 255));
        draw_text(buffer, preview_status_label(preview.status),
                  214.0f, 96.0f, 0.32f,
                  preview.valid() ? C2D_Color32(112, 225, 164, 255)
                                  : C2D_Color32(246, 126, 99, 255));

        C2D_DrawRectSolid(16.0f, 120.0f, 0.18f, 280.0f, 25.0f,
                          C2D_Color32(32, 42, 43, 255));
        draw_text(buffer, "STEROWANIE", 22.0f, 121.0f, 0.31f,
                  C2D_Color32(231, 174, 68, 255));
        draw_text(buffer,
                  "D-PAD MIEJSCE  L/R TYP  A OK  B WSTECZ",
                  84.0f, 126.0f, 0.25f,
                  C2D_Color32(244, 239, 220, 255));

        draw_button(atlas, buffer,
                    kPrimaryActionBounds, kPrimaryActionId,
                    -1, -1, preview.valid(), true,
                    UiIcon::Build,
                    preview.valid() ? "POTWIERDZ" : "NIEDOSTEPNE");
        draw_button(atlas, buffer,
                    kBuildActionBounds, kBuildActionId,
                    -1, -1, true, false,
                    room_type_icon(type), "TYP L/R");
        draw_button(atlas, buffer,
                    kSaveActionBounds, kSaveActionId,
                    -1, -1, true, false,
                    UiIcon::Save, "ANULUJ B");
    } else {
        const auto& selected = state.room_entries[
            static_cast<std::size_t>(state.selected_room)];
        atlas.draw_icon(room_type_icon(selected.type),
                        20.0f, 48.0f, 22.0f, 22.0f, 0.35f);
        draw_text(buffer, "POKOJ", 50.0f, 44.0f, 0.36f,
                  C2D_Color32(151, 168, 171, 255));
        draw_text(buffer, room_type_label(selected.type),
                  50.0f, 57.0f, 0.50f,
                  C2D_Color32(246, 193, 82, 255));

        C2D_DrawRectSolid(16.0f, 78.0f, 0.18f, 88.0f, 35.0f,
                          C2D_Color32(24, 39, 44, 255));
        C2D_DrawRectSolid(112.0f, 78.0f, 0.18f, 88.0f, 35.0f,
                          C2D_Color32(24, 39, 44, 255));
        C2D_DrawRectSolid(208.0f, 78.0f, 0.18f, 88.0f, 35.0f,
                          C2D_Color32(24, 39, 44, 255));
        draw_text(buffer, "EFEKT", 22.0f, 81.0f, 0.34f,
                  C2D_Color32(137, 155, 163, 255));
        draw_text(buffer, production_label(selected.type),
                  22.0f, 96.0f, 0.39f,
                  C2D_Color32(238, 198, 99, 255));
        draw_text(buffer, "ZALOGA", 118.0f, 81.0f, 0.34f,
                  C2D_Color32(137, 155, 163, 255));
        draw_text(buffer,
                  session.selected_has_worker() ? "1 / 3" : "0 / 3",
                  118.0f, 96.0f, 0.46f,
                  C2D_Color32(112, 207, 159, 255));
        draw_text(buffer, "ZAPAS", 214.0f, 81.0f, 0.34f,
                  C2D_Color32(137, 155, 163, 255));
        char cycle_status[32];
        const int seconds_left =
            (deep_shelter::gameplay::kPlayableProductionCycleSteps -
             session.selected_progress() + 59) /
            60;
        if (session.selected_has_worker()) {
            std::snprintf(cycle_status,
                          sizeof(cycle_status),
                          "%d/30 %ds",
                          session.selected_stored(),
                          seconds_left);
        } else {
            std::snprintf(cycle_status,
                          sizeof(cycle_status),
                          "%d/30 --",
                          session.selected_stored());
        }
        draw_text(buffer, cycle_status,
                  214.0f, 96.0f, 0.42f,
                  C2D_Color32(112, 207, 159, 255));

        C2D_DrawRectSolid(16.0f, 120.0f, 0.18f, 280.0f, 25.0f,
                          C2D_Color32(32, 42, 43, 255));
        draw_text(buffer, "DALEJ", 22.0f, 121.0f, 0.34f,
                  C2D_Color32(231, 174, 68, 255));
        draw_text(buffer,
                  notice != nullptr ? notice : session.next_step(),
                  68.0f, 126.0f, 0.36f,
                  C2D_Color32(244, 239, 220, 255));

        const int focused_id = ui.focused_id().value_or(-1);
        const int pressed_id = ui.pressed_id().value_or(-1);
        const PrimaryAction primary = session.primary_action();
        draw_button(atlas, buffer,
                    kPrimaryActionBounds, kPrimaryActionId,
                    focused_id, pressed_id,
                    primary != PrimaryAction::Wait, true,
                    primary_icon(primary), primary_label(primary));
        draw_button(atlas, buffer,
                    kBuildActionBounds, kBuildActionId,
                    focused_id, pressed_id,
                    state.rooms <
                        deep_shelter::gameplay::kPlayableRoomCapacity,
                    false, UiIcon::Build, "BUDUJ");
        draw_button(atlas, buffer,
                    kSaveActionBounds, kSaveActionId,
                    focused_id, pressed_id, true, false,
                    UiIcon::Save, "ZAPIS");
    }
    C2D_Flush();
}

InputFrame read_ui_input(u32 down, u32 held, u32 up) {
    InputFrame input;
    input.left = (down & KEY_DLEFT) != 0;
    input.right = (down & KEY_DRIGHT) != 0;
    input.up = (down & KEY_DUP) != 0;
    input.down = (down & KEY_DDOWN) != 0;
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

struct UiAvailability {
    int primary = -1;
    int build = -1;
};

void sync_ui_availability(UiTree& ui,
                          const PlayableShelterSession& session,
                          UiAvailability& previous) {
    using namespace deep_shelter::ui::shelter_hud;
    const auto& state = session.state();

    const PrimaryAction primary = session.primary_action();
    const int primary_value = static_cast<int>(primary);
    if (primary_value != previous.primary) {
        if (primary == PrimaryAction::Wait) {
            ui.set_enabled(kPrimaryActionId,
                           false,
                           "Produkcja trwa.",
                           "Poczekaj na zakonczenie cyklu.");
        } else {
            ui.set_enabled(kPrimaryActionId, true);
        }
        previous.primary = primary_value;
    }

    const int build_value =
        state.rooms >= deep_shelter::gameplay::kPlayableRoomCapacity ? 1 : 0;
    if (build_value != previous.build) {
        if (build_value == 1) {
            ui.set_enabled(kBuildActionId,
                           false,
                           "Brak wolnej komorki.",
                           "Schron wykorzystuje cala siatke.");
        } else {
            ui.set_enabled(kBuildActionId, true);
        }
        previous.build = build_value;
    }
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
    C3D_RenderTarget* top_left =
        C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* top_right =
        C2D_CreateScreenTarget(GFX_TOP, GFX_RIGHT);
    C3D_RenderTarget* bottom =
        C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    C2D_TextBuf text_buffer = C2D_TextBufNew(4096);
    if (top_left == nullptr || top_right == nullptr || bottom == nullptr ||
        text_buffer == nullptr) {
        if (text_buffer != nullptr) C2D_TextBufDelete(text_buffer);
        C2D_Fini();
        C3D_Fini();
        gfxExit();
        return 3;
    }

    static Scene3DRenderer scene_renderer;
    static DwellerBillboardRenderer dweller_renderer;
    static GlowPassRenderer glow_renderer;
    static GeneratedUiRenderer ui_renderer;
    const bool renderer_ready = scene_renderer.initialize();
    const bool dwellers_ready = dweller_renderer.initialize();
    const bool glow_ready = glow_renderer.initialize();
    const bool ui_atlas_ready = ui_renderer.initialize();

    ShelterCamera camera(
        {deep_shelter::render::layout::kWorldWidth,
         deep_shelter::render::layout::kWorldHeight},
        {400.0f, 240.0f});
    UiTree ui;
    using namespace deep_shelter::ui::shelter_hud;
    ui.add({kPrimaryActionId, kPrimaryActionBounds,
            true, true, true, {}, {}});
    ui.add({kBuildActionId, kBuildActionBounds,
            true, true, true, {}, {}});
    ui.add({kSaveActionId, kSaveActionBounds,
            true, true, true, {}, {}});

    PlayableShelterState initial_state;
    PlayableShelterSession session(initial_state);
    center_on_selected(camera, session);
    deep_shelter::core::FixedStepClock simulation_clock(
        1.0 / 60.0, 15, 0.25);
    UiAvailability ui_availability;
    sync_ui_availability(ui, session, ui_availability);
    const char* startup_notice = nullptr;
    if (!renderer_ready) {
        startup_notice = "Blad renderera 3D - tryb diagnostyczny.";
    } else if (!dwellers_ready) {
        startup_notice = "Blad postaci - schron dziala bez mieszkanca.";
    } else if (!glow_ready) {
        startup_notice = "Blad swiatla - gra dziala bez poswiaty.";
    } else if (!ui_atlas_ready) {
        startup_notice = "Blad atlasu UI - interfejs awaryjny.";
    }

    char notice[96] = {};
    u64 notice_until_ms = 0;
    std::uint32_t animation_tick = 0;
    u64 previous_frame_ms = osGetTime();
    bool build_mode = false;

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
        if (held & KEY_X) camera.zoom_by(-0.025f);
        if (held & KEY_Y) camera.zoom_by(0.025f);

        const u64 current_frame_ms = osGetTime();
        const double frame_seconds =
            static_cast<double>(current_frame_ms - previous_frame_ms) /
            1000.0;
        previous_frame_ms = current_frame_ms;
        simulation_clock.advance(frame_seconds, [&](double) {
            session.fixed_step();
            ++animation_tick;
        });

        if (build_mode) {
            int column_delta = 0;
            int floor_delta = 0;
            if (down & KEY_DLEFT) column_delta = -1;
            if (down & KEY_DRIGHT) column_delta = 1;
            if (down & KEY_DUP) floor_delta = -1;
            if (down & KEY_DDOWN) floor_delta = 1;
            if (column_delta != 0 || floor_delta != 0) {
                (void)session.move_build_cursor(
                    column_delta, floor_delta);
                center_on_cell(camera,
                               session.build_cursor_column(),
                               session.build_cursor_floor());
            }
            if (down & KEY_L) {
                (void)session.select_build_type(adjacent_room_type(
                    session.selected_build_type(), -1));
            }
            if (down & KEY_R) {
                (void)session.select_build_type(adjacent_room_type(
                    session.selected_build_type(), 1));
            }
            if (down & KEY_A) {
                const BuildResult result = session.confirm_build();
                set_notice(notice,
                           sizeof(notice),
                           build_result_notice(result));
                notice_until_ms = current_frame_ms + 1800u;
                if (result == BuildResult::Built) {
                    build_mode = false;
                    center_on_selected(camera, session);
                    ui_availability = UiAvailability{};
                }
            }
            if (down & KEY_B) {
                build_mode = false;
                set_notice(notice,
                           sizeof(notice),
                           "Anulowano budowe.");
                notice_until_ms = current_frame_ms + 1200u;
                center_on_selected(camera, session);
            }
        } else {
            if (down & KEY_L) {
                if (session.select_previous_room()) {
                    center_on_selected(camera, session);
                }
                notice_until_ms = 0;
            }
            if (down & KEY_R) {
                if (session.select_next_room()) {
                    center_on_selected(camera, session);
                }
                notice_until_ms = 0;
            }

            if (down & KEY_SELECT) {
                const auto loaded =
                    deep_shelter::gameplay::load_playable_state(kSavePath);
                if (loaded.status == PlayableSaveStatus::Ok) {
                    session = PlayableShelterSession(loaded.state);
                    set_notice(notice,
                               sizeof(notice),
                               loaded.used_backup
                                   ? "Wczytano kopie zapasowa."
                                   : "Wczytano stan schronu.");
                    center_on_selected(camera, session);
                } else {
                    set_notice(notice,
                               sizeof(notice),
                               "Brak zgodnego zapisu schronu.");
                }
                notice_until_ms = current_frame_ms + 1800u;
                ui_availability = UiAvailability{};
            }

            sync_ui_availability(ui, session, ui_availability);
            const auto action = ui.route(read_ui_input(down, held, up));
            if (action && action->type == UiActionType::Activate) {
                if (action->control_id == kPrimaryActionId) {
                    const PrimaryAction current = session.primary_action();
                    if (current == PrimaryAction::Assign) {
                        const bool assigned =
                            session.assign_resident_to_room(
                                0u, session.state().selected_room);
                        set_notice(
                            notice,
                            sizeof(notice),
                            assigned
                                ? "Mieszkaniec jest w drodze do pracy."
                                : "Brak legalnej trasy do pokoju.");
                    } else if (current == PrimaryAction::Collect) {
                        const CollectResult result =
                            session.collect_selected_room();
                        set_notice(
                            notice,
                            sizeof(notice),
                            result == CollectResult::Collected
                                ? "Odebrano produkcje."
                                : "Brak gotowej produkcji.");
                    }
                    notice_until_ms = current_frame_ms + 1800u;
                } else if (action->control_id == kBuildActionId) {
                    build_mode = true;
                    notice_until_ms = 0;
                    center_on_cell(camera,
                                   session.build_cursor_column(),
                                   session.build_cursor_floor());
                } else if (action->control_id == kSaveActionId) {
                    const PlayableSaveStatus status =
                        deep_shelter::gameplay::save_playable_state(
                            kSavePath, session.state());
                    set_notice(
                        notice,
                        sizeof(notice),
                        status == PlayableSaveStatus::Ok
                            ? "Zapisano stan schronu."
                            : "Blad zapisu - sprawdz karte SD.");
                    notice_until_ms = current_frame_ms + 1800u;
                }
            } else if (action &&
                       action->type == UiActionType::ShowDisabledReason) {
                set_notice(notice,
                           sizeof(notice),
                           action->message.c_str());
                notice_until_ms = current_frame_ms + 1800u;
            }
        }

        sync_ui_availability(ui, session, ui_availability);
        const float stereo =
            osGet3DSliderState() *
            deep_shelter::render::kShelterStereoFullSlider;
        const ShelterSceneState3D scene_state =
            make_scene_state(session, build_mode, animation_tick);
        RenderStats left_stats{};
        RenderStats right_stats{};
        C2D_TextBufClear(text_buffer);

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        if (ui_atlas_ready) {
            const char* active_notice =
                startup_notice != nullptr
                    ? startup_notice
                    : (current_frame_ms < notice_until_ms
                           ? notice
                           : nullptr);
            draw_bottom(bottom,
                        text_buffer,
                        session,
                        ui,
                        ui_renderer,
                        active_notice,
                        build_mode);
        } else {
            C2D_Prepare();
            C2D_TargetClear(bottom,
                            C2D_Color32(15, 30, 39, 255));
            C2D_SceneBegin(bottom);
            draw_text(text_buffer,
                      startup_notice != nullptr
                          ? startup_notice
                          : (build_mode
                                 ? "TRYB BUDOWY"
                                 : session.next_step()),
                      12.0f,
                      84.0f,
                      0.48f,
                      C2D_Color32(255, 255, 255, 255));
            C2D_Flush();
        }
        if (renderer_ready) {
            scene_renderer.draw(top_left,
                                camera,
                                -stereo,
                                scene_state,
                                left_stats);
            if (dwellers_ready) {
                dweller_renderer.draw(top_left,
                                       camera,
                                       -stereo,
                                       scene_state,
                                       left_stats);
            }
            if (glow_ready) {
                glow_renderer.draw(top_left,
                                   camera,
                                   -stereo,
                                   scene_state);
            }
            scene_renderer.draw(top_right,
                                camera,
                                stereo,
                                scene_state,
                                right_stats);
            if (dwellers_ready) {
                dweller_renderer.draw(top_right,
                                       camera,
                                       stereo,
                                       scene_state,
                                       right_stats);
            }
            if (glow_ready) {
                glow_renderer.draw(top_right,
                                   camera,
                                   stereo,
                                   scene_state);
            }
        } else {
            C3D_RenderTargetClear(top_left,
                                  C3D_CLEAR_COLOR,
                                  kRendererDiagnosticColor,
                                  0);
            C3D_FrameDrawOn(top_left);
            C3D_RenderTargetClear(top_right,
                                  C3D_CLEAR_COLOR,
                                  kRendererDiagnosticColor,
                                  0);
            C3D_FrameDrawOn(top_right);
        }
        C3D_FrameEnd(0);
    }

    ui_renderer.shutdown();
    glow_renderer.shutdown();
    dweller_renderer.shutdown();
    scene_renderer.shutdown();
    C2D_TextBufDelete(text_buffer);
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
