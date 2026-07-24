#include "ui/UiFramework.hpp"

#include <cassert>

using namespace deep_shelter::ui;

int main() {
    UiTree ui;
    assert(ui.add({1, {10, 10, 80, 40}, true, true, true, {}, {}}));
    assert(ui.add({2, {110, 10, 80, 40}, true, true, true, {}, {}}));
    assert(ui.add({3, {110, 70, 80, 40}, true, false, true,
                   "Brak energii.", "Zbuduj generator."}));
    assert(!ui.add({1, {10, 60, 40, 30}, true, true, true, {}, {}}));
    assert(!ui.add({4, {0, 0, 20, 20}, true, true, true, {}, {}}));

    assert(ui.focused_id() && *ui.focused_id() == 1);
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

    InputFrame release;
    release.touch_released = true;
    release.touch_x = 20;
    release.touch_y = 20;
    action = ui.route(release);
    assert(action && action->type == UiActionType::Activate && action->control_id == 1);

    press = {};
    press.touch_pressed = true;
    press.touch_x = 120;
    press.touch_y = 20;
    assert(!ui.route(press));
    release = {};
    release.touch_released = true;
    release.touch_x = 300;
    release.touch_y = 220;
    assert(!ui.route(release)); // release outside cancels activation

    press.touch_x = 120;
    press.touch_y = 20;
    assert(!ui.route(press));
    InputFrame drag;
    drag.touch_held = true;
    drag.touch_x = 140;
    drag.touch_y = 55;
    action = ui.route(drag);
    assert(action && action->type == UiActionType::Drag && action->control_id == 2);

    InputFrame cancel;
    cancel.cancel = true;
    action = ui.route(cancel);
    assert(action && action->type == UiActionType::Cancel);

    assert(ui.focus(2));
    assert(ui.remove(2));
    assert(ui.focused_id() && *ui.focused_id() == 1);
    ui.clear();
    assert(!ui.focused_id());
    return 0;
}
