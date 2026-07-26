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
#include "render/Scene3DRenderer.hpp"
#include "render/ShelterCamera.hpp"
#include "render/ShelterSceneLayout.hpp"
#include "ui/GeneratedUiRenderer.hpp"
#include "ui/ShelterHudLayout.hpp"
#include "ui/UiFramework.hpp"

namespace {

constexpr const char* kSavePath = "sdmc:/DeepShelter3D_demo";
constexpr u32 kRendererDiagnosticColor = 0x9A2020FF;

using deep_shelter::assets::UiButtonState;
using deep_shelter::assets::UiIcon;
using deep_shelter::gameplay::BuildResult;
using deep_shelter::gameplay::CollectResult;
using deep_shelter::gameplay::PlayableSaveStatus;
using deep_shelter::gameplay::PlayableShelterSession;
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
    C2D_DrawRectSolid(x, y, 0.20f, width, primary ? 5.0f : 3.0f,
                      border);
    C2D_DrawRectSolid(x, y + height - 2.0f, 0.20f, width, 2.0f,
                      border);
    C2D_DrawRectSolid(x, y, 0.20f, 2.0f, height, border);
    C2D_DrawRectSolid(x + width - 2.0f, y, 0.20f, 2.0f, height,
                      border);
    const float icon_size = primary ? 30.0f : 20.0f;
    atlas.draw_icon(icon,
                    x + (primary ? 15.0f : 9.0f),
                    y + (height - icon_size) * 0.5f,
                    icon_size,
                    icon_size,
                    0.35f);
    if (primary) {
        draw_text(buffer, "AKCJA POKOJU", x + 57.0f, y + 8.0f, 0.38f,
                  C2D_Color32(168, 181, 181, 255));
    }
    draw_text(buffer,
              label,
              x + (primary ? 57.0f : 35.0f),
              y + (primary ? 33.0f : 8.0f),
              primary ? 0.58f : 0.44f,
              enabled ? C2D_Color32(244, 239, 220, 255)
                      : C2D_Color32(105, 114, 117, 255));
}

UiIcon room_icon(int room_index) {
    switch ((room_index % 6 + 6) % 6) {
        case 0: return UiIcon::Power;
        case 1: return UiIcon::Food;
        case 2: return UiIcon::Water;
        case 3: return UiIcon::Work;
        case 4: return UiIcon::Credits;
        default: return UiIcon::Work;
    }
}

const char* production_label(int room_index) {
    switch ((room_index % 6 + 6) % 6) {
        case 0: return "ENERGIA +5";
        case 1: return "ZYWNOSC +5";
        case 2: return "WODA +5";
        default: return "KREDYTY";
    }
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
                 const char* notice) {
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
                      C2D_Color32(180, 128, 48, 255));
    atlas.draw_icon(room_icon(state.selected_room),
                    20.0f, 48.0f, 22.0f, 22.0f, 0.35f);
    draw_text(buffer, "POKOJ", 50.0f, 44.0f, 0.36f,
              C2D_Color32(151, 168, 171, 255));
    draw_text(buffer, room_label(state.selected_room), 50.0f, 57.0f, 0.50f,
              C2D_Color32(246, 193, 82, 255));

    C2D_DrawRectSolid(16.0f, 78.0f, 0.18f, 88.0f, 35.0f,
                      C2D_Color32(24, 39, 44, 255));
    C2D_DrawRectSolid(112.0f, 78.0f, 0.18f, 88.0f, 35.0f,
                      C2D_Color32(24, 39, 44, 255));
    C2D_DrawRectSolid(208.0f, 78.0f, 0.18f, 88.0f, 35.0f,
                      C2D_Color32(24, 39, 44, 255));
    draw_text(buffer, "EFEKT", 22.0f, 81.0f, 0.34f,
              C2D_Color32(137, 155, 163, 255));
    draw_text(buffer, production_label(state.selected_room),
              22.0f, 96.0f, 0.39f, C2D_Color32(238, 198, 99, 255));
    draw_text(buffer, "ZALOGA", 118.0f, 81.0f, 0.34f,
              C2D_Color32(137, 155, 163, 255));
    draw_text(buffer, session.selected_has_worker() ? "1 / 1" : "0 / 1",
              118.0f, 96.0f, 0.46f, C2D_Color32(112, 207, 159, 255));
    draw_text(buffer, "ZAPAS", 214.0f, 81.0f, 0.34f,
              C2D_Color32(137, 155, 163, 255));
    char cycle_status[32];
    const int seconds_left =
        (deep_shelter::gameplay::kPlayableProductionCycleSteps -
         session.selected_progress() + 59) /
        60;
    if (session.selected_has_worker()) {
        std::snprintf(cycle_status, sizeof(cycle_status), "%d/30 %ds",
                      session.selected_stored(), seconds_left);
    } else {
        std::snprintf(cycle_status, sizeof(cycle_status), "%d/30 --",
                      session.selected_stored());
    }
    draw_text(buffer, cycle_status, 214.0f, 96.0f, 0.42f,
              C2D_Color32(112, 207, 159, 255));

    C2D_DrawRectSolid(16.0f, 120.0f, 0.18f, 280.0f, 25.0f,
                      C2D_Color32(32, 42, 43, 255));
    draw_text(buffer, "DALEJ", 22.0f, 121.0f, 0.34f,
              C2D_Color32(231, 174, 68, 255));
    draw_text(buffer,
              notice != nullptr ? notice : session.next_step(),
              68.0f,
              126.0f,
              0.36f,
              C2D_Color32(244, 239, 220, 255));

    const int focused_id = ui.focused_id().value_or(-1);
    const int pressed_id = ui.pressed_id().value_or(-1);
    const PrimaryAction primary = session.primary_action();
    draw_button(atlas,
                buffer,
                kPrimaryActionBounds,
                kPrimaryActionId,
                focused_id,
                pressed_id,
                primary != PrimaryAction::Wait,
                true,
                primary_icon(primary),
                primary_label(primary));
    draw_button(atlas,
                buffer,
                kBuildActionBounds,
                kBuildActionId,
                focused_id,
                pressed_id,
                state.rooms < deep_shelter::gameplay::kPlayableMaxRooms &&
                    state.credits >= 100,
                false,
                UiIcon::Build,
                "BUDUJ");
    draw_button(atlas,
                buffer,
                kSaveActionBounds,
                kSaveActionId,
                focused_id,
                pressed_id,
                true,
                false,
                UiIcon::Save,
                "ZAPIS");
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

    int build_value = 0;
    if (state.rooms >= deep_shelter::gameplay::kPlayableMaxRooms) {
        build_value = 1;
    } else if (state.credits < 100) {
        build_value = 2;
    }
    if (build_value != previous.build) {
        if (build_value == 1) {
            ui.set_enabled(kBuildActionId,
                           false,
                           "Brak wolnego modulu.",
                           "Schron ma juz 6 pokoi.");
        } else if (build_value == 2) {
            ui.set_enabled(kBuildActionId,
                           false,
                           "Za malo kredytow.",
                           "Odbierz produkcje.");
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

    // Renderers own fixed buffers. Keep them in static storage rather than using
    // the small 3DS main-thread stack.
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

    PlayableShelterSession session;
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
        if (down & KEY_L) {
            (void)session.select_previous_room();
            notice_until_ms = 0;
        }
        if (down & KEY_R) {
            (void)session.select_next_room();
            notice_until_ms = 0;
        }

        const u64 current_frame_ms = osGetTime();
        const double frame_seconds =
            static_cast<double>(current_frame_ms - previous_frame_ms) / 1000.0;
        previous_frame_ms = current_frame_ms;
        simulation_clock.advance(frame_seconds, [&](double) {
            session.fixed_step();
            ++animation_tick;
        });
        sync_ui_availability(ui, session, ui_availability);

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
            } else {
                set_notice(notice,
                           sizeof(notice),
                           "Brak zgodnego zapisu schronu.");
            }
            notice_until_ms = current_frame_ms + 1800u;
            ui_availability = UiAvailability{};
            sync_ui_availability(ui, session, ui_availability);
        }

        const auto action = ui.route(read_ui_input(down, held, up));
        if (action && action->type == UiActionType::Activate) {
            if (action->control_id == kPrimaryActionId) {
                const PrimaryAction current = session.primary_action();
                if (current == PrimaryAction::Assign) {
                    session.assign_selected_room();
                    set_notice(notice, sizeof(notice),
                               "Mieszkaniec rozpoczal prace.");
                } else if (current == PrimaryAction::Collect) {
                    const CollectResult result =
                        session.collect_selected_room();
                    set_notice(notice,
                               sizeof(notice),
                               result == CollectResult::Collected
                                   ? "Odebrano produkcje."
                                   : "Brak gotowej produkcji.");
                }
                notice_until_ms = current_frame_ms + 1800u;
            }
            if (action->control_id == kBuildActionId) {
                const BuildResult result = session.build_room();
                const char* message = "Zbudowano nowy pokoj.";
                if (result == BuildResult::Full) {
                    message = "Schron ma juz 6 pokoi.";
                } else if (result == BuildResult::NotEnoughCredits) {
                    message = "Za malo kredytow.";
                }
                set_notice(notice, sizeof(notice), message);
                notice_until_ms = current_frame_ms + 1800u;
            }
            if (action->control_id == kSaveActionId) {
                const PlayableSaveStatus status =
                    deep_shelter::gameplay::save_playable_state(
                        kSavePath, session.state());
                set_notice(notice,
                           sizeof(notice),
                           status == PlayableSaveStatus::Ok
                               ? "Zapisano stan schronu."
                               : "Blad zapisu - sprawdz karte SD.");
                notice_until_ms = current_frame_ms + 1800u;
            }
        } else if (action && action->type == UiActionType::ShowDisabledReason) {
            set_notice(notice, sizeof(notice), action->message.c_str());
            notice_until_ms = current_frame_ms + 1800u;
        } else if (action && action->type == UiActionType::Cancel) {
            set_notice(notice, sizeof(notice), "Anulowano.");
            notice_until_ms = current_frame_ms + 1000u;
        }
        sync_ui_availability(ui, session, ui_availability);
        const auto& state = session.state();
        const float stereo = osGet3DSliderState() * 0.035f;
        const ShelterSceneState3D scene_state{state.rooms,
                                               state.selected_room,
                                               session.selected_stored(),
                                               state.assigned_room,
                                               animation_tick};
        RenderStats left_stats{};
        RenderStats right_stats{};
        C2D_TextBufClear(text_buffer);

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        if (ui_atlas_ready) {
            const char* active_notice =
                startup_notice != nullptr
                    ? startup_notice
                    : (current_frame_ms < notice_until_ms ? notice : nullptr);
            draw_bottom(bottom,
                        text_buffer,
                        session,
                        ui,
                        ui_renderer,
                        active_notice);
        } else {
            C2D_Prepare();
            C2D_TargetClear(bottom, C2D_Color32(15, 30, 39, 255));
            C2D_SceneBegin(bottom);
            draw_text(text_buffer,
                      startup_notice != nullptr
                          ? startup_notice
                          : session.next_step(),
                      12.0f,
                      84.0f,
                      0.48f,
                      C2D_Color32(255, 255, 255, 255));
            C2D_Flush();
        }
        if (renderer_ready) {
            scene_renderer.draw(top_left, camera, -stereo, scene_state, left_stats);
            if (dwellers_ready) {
                dweller_renderer.draw(top_left,
                                       camera,
                                       -stereo,
                                       scene_state,
                                       left_stats);
            }
            if (glow_ready) glow_renderer.draw(top_left, camera, -stereo, scene_state);
            scene_renderer.draw(top_right, camera, stereo, scene_state, right_stats);
            if (dwellers_ready) {
                dweller_renderer.draw(top_right,
                                       camera,
                                       stereo,
                                       scene_state,
                                       right_stats);
            }
            if (glow_ready) glow_renderer.draw(top_right, camera, stereo, scene_state);
        } else {
            C3D_RenderTargetClear(top_left, C3D_CLEAR_COLOR, kRendererDiagnosticColor, 0);
            C3D_FrameDrawOn(top_left);
            C3D_RenderTargetClear(top_right, C3D_CLEAR_COLOR, kRendererDiagnosticColor, 0);
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
