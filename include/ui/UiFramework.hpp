#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace deep_shelter::ui {

struct Rect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    [[nodiscard]] bool contains(int px, int py) const noexcept;
};

enum class UiActionType {
    None,
    Activate,
    Cancel,
    FocusChanged,
    Scroll,
    Drag,
    ShowDisabledReason,
};

struct UiAction {
    UiActionType type = UiActionType::None;
    int control_id = -1;
    int value = 0;
    std::string message;
};

struct Control {
    int id = -1;
    Rect bounds{};
    bool focusable = true;
    bool enabled = true;
    bool visible = true;
    std::string disabled_reason;
    std::string next_step;
};

struct InputFrame {
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
    bool confirm = false;
    bool cancel = false;
    bool touch_pressed = false;
    bool touch_held = false;
    bool touch_released = false;
    int touch_x = 0;
    int touch_y = 0;
};

class UiTree {
public:
    explicit UiTree(Rect safe_area = {4, 4, 312, 232});

    bool add(Control control);
    bool remove(int id);
    void clear() noexcept;
    bool set_enabled(int id, bool enabled, std::string reason = {}, std::string next_step = {});
    bool focus(int id) noexcept;

    [[nodiscard]] std::optional<int> focused_id() const noexcept;
    [[nodiscard]] std::optional<int> pressed_id() const noexcept;
    [[nodiscard]] std::optional<Control> control(int id) const;
    [[nodiscard]] std::optional<UiAction> route(const InputFrame& input);

private:
    std::optional<std::size_t> index_of(int id) const noexcept;
    std::optional<std::size_t> hit_test(int x, int y) const noexcept;
    bool move_focus(int dx, int dy) noexcept;
    void repair_focus() noexcept;
    UiAction activate(std::size_t index) const;

    Rect safe_area_;
    std::vector<Control> controls_;
    std::optional<std::size_t> focused_;
    std::optional<std::size_t> captured_;
    int press_x_ = 0;
    int press_y_ = 0;
};

}  // namespace deep_shelter::ui
