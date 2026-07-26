#include "ui/UiFramework.hpp"
#include "ui/ShelterHudLayout.hpp"

#include <cassert>

using namespace deep_shelter::ui;

int main() {
    using namespace deep_shelter::ui::shelter_hud;

    UiTree shelter_ui;
    assert(shelter_ui.add({kPrimaryActionId, kPrimaryActionBounds,
                           true, true, true, {}, {}}));
    assert(shelter_ui.add({kBuildActionId, kBuildActionBounds,
                           true, true, true, {}, {}}));
    assert(shelter_ui.add({kSaveActionId, kSaveActionBounds,
                           true, true, true, {}, {}}));
    struct ExpectedControl {
        int id;
        Rect bounds;
    };
    for (const auto expected : {
             ExpectedControl{kPrimaryActionId, kPrimaryActionBounds},
             ExpectedControl{kBuildActionId, kBuildActionBounds},
             ExpectedControl{kSaveActionId, kSaveActionBounds},
         }) {
        const int left = expected.bounds.x;
        const int right =
            expected.bounds.x + expected.bounds.width - 1;
        const int top = expected.bounds.y;
        const int bottom =
            expected.bounds.y + expected.bounds.height - 1;
        for (const auto point : {
                 Rect{left, top, 0, 0},
                 Rect{right, top, 0, 0},
                 Rect{left, bottom, 0, 0},
                 Rect{right, bottom, 0, 0},
                 Rect{left + expected.bounds.width / 2,
                      top + expected.bounds.height / 2,
                      0,
                      0},
             }) {
            InputFrame press;
            press.touch_pressed = true;
            press.touch_x = point.x;
            press.touch_y = point.y;
            assert(!shelter_ui.route(press));

            InputFrame release;
            release.touch_released = true;
            release.touch_x = point.x;
            release.touch_y = point.y;
            const auto action = shelter_ui.route(release);
            assert(action && action->type == UiActionType::Activate);
            assert(action->control_id == expected.id);
        }
    }

    for (const auto point : {
             Rect{9, 160, 0, 0},
             Rect{204, 194, 0, 0},
             Rect{211, 175, 0, 0},
             Rect{311, 175, 0, 0},
             Rect{250, 193, 0, 0},
             Rect{250, 228, 0, 0},
         }) {
        InputFrame press;
        press.touch_pressed = true;
        press.touch_x = point.x;
        press.touch_y = point.y;
        assert(!shelter_ui.route(press));
        InputFrame release;
        release.touch_released = true;
        release.touch_x = point.x;
        release.touch_y = point.y;
        assert(!shelter_ui.route(release));
    }

    assert(shelter_ui.set_enabled(
        kPrimaryActionId, false, "wait", "collect"));
    InputFrame disabled_press;
    disabled_press.touch_pressed = true;
    disabled_press.touch_x = kPrimaryActionBounds.x + 10;
    disabled_press.touch_y = kPrimaryActionBounds.y + 10;
    assert(!shelter_ui.route(disabled_press));
    InputFrame disabled_release;
    disabled_release.touch_released = true;
    disabled_release.touch_x = disabled_press.touch_x;
    disabled_release.touch_y = disabled_press.touch_y;
    const auto disabled = shelter_ui.route(disabled_release);
    assert(disabled);
    assert(disabled->type == UiActionType::ShowDisabledReason);
    assert(disabled->control_id == kPrimaryActionId);

    UiTree ui;
    assert(ui.add({1, {10, 10, 80, 40}, true, true, true, {}, {}}));
    assert(ui.add({2, {110, 10, 80, 40}, true, true, true, {}, {}}));
    assert(ui.add({3, {110, 70, 80, 40}, true, false, true,
                   "Brak energii.", "Zbuduj generator."}));
    assert(!ui.add({1, {10, 60, 40, 30}, true, true, true, {}, {}}));
    assert(!ui.add({4, {0, 0, 20, 20}, true, true, true, {}, {}}));

    assert(ui.focused_id() && *ui.focused_id() == 1);
    assert(!ui.pressed_id());
    auto action = ui.route({false, false, false, true});
    assert(action && action->type == UiActionType::FocusChanged && action->control_id == 2);
    action = ui.route({false, true});
    assert(action && action->type == UiActionType::FocusChanged && action->control_id == 3);
    action = ui.route({false, false, false, false, true});
    assert(action && action->type == UiActionType::ShowDisabledReason);
    assert(action->message.find("Brak energii") != std::string::npos);
    assert(action->message.find("Zbuduj generator") != std::string::npos);

    InputFrame press;
    press.confirm = true;
    press.touch_pressed = true;
    press.touch_x = 20;
    press.touch_y = 20;
    action = ui.route(press);
    assert(!action); // touch captures the frame; A cannot double-activate
    assert(ui.pressed_id() && *ui.pressed_id() == 1);

    InputFrame release;
    release.touch_released = true;
    release.touch_x = 20;
    release.touch_y = 20;
    action = ui.route(release);
    assert(action && action->type == UiActionType::Activate && action->control_id == 1);
    assert(!ui.pressed_id());

    press = {};
    press.touch_pressed = true;
    press.touch_x = 120;
    press.touch_y = 20;
    assert(!ui.route(press));
    assert(ui.pressed_id() && *ui.pressed_id() == 2);
    release = {};
    release.touch_released = true;
    release.touch_x = 300;
    release.touch_y = 220;
    assert(!ui.route(release)); // release outside cancels activation
    assert(!ui.pressed_id());

    press.touch_x = 120;
    press.touch_y = 20;
    assert(!ui.route(press));
    InputFrame drag;
    drag.touch_held = true;
    drag.touch_x = 140;
    drag.touch_y = 55;
    action = ui.route(drag);
    assert(action && action->type == UiActionType::Drag && action->control_id == 2);
    assert(ui.pressed_id() && *ui.pressed_id() == 2);

    InputFrame cancel;
    cancel.cancel = true;
    action = ui.route(cancel);
    assert(action && action->type == UiActionType::Cancel);
    assert(!ui.pressed_id());

    assert(ui.focus(2));
    assert(ui.remove(2));
    assert(ui.focused_id() && *ui.focused_id() == 1);
    ui.clear();
    assert(!ui.focused_id());
    assert(!ui.pressed_id());
    return 0;
}
