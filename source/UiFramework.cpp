#include "ui/UiFramework.hpp"

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <utility>

namespace deep_shelter::ui {

bool Rect::contains(int px, int py) const noexcept {
    return width > 0 && height > 0 && px >= x && py >= y && px < x + width && py < y + height;
}

UiTree::UiTree(Rect safe_area) : safe_area_(safe_area) {}

bool UiTree::add(Control control) {
    if (control.id < 0 || index_of(control.id).has_value()) return false;
    if (control.bounds.width <= 0 || control.bounds.height <= 0) return false;
    if (!safe_area_.contains(control.bounds.x, control.bounds.y) ||
        !safe_area_.contains(control.bounds.x + control.bounds.width - 1,
                             control.bounds.y + control.bounds.height - 1)) {
        return false;
    }
    controls_.push_back(std::move(control));
    repair_focus();
    return true;
}

bool UiTree::remove(int id) {
    const auto index = index_of(id);
    if (!index) return false;
    controls_.erase(controls_.begin() + static_cast<std::ptrdiff_t>(*index));
    if (captured_ && *captured_ == *index) captured_.reset();
    if (captured_ && *captured_ > *index) --(*captured_);
    focused_.reset();
    repair_focus();
    return true;
}

void UiTree::clear() noexcept {
    controls_.clear();
    focused_.reset();
    captured_.reset();
}

bool UiTree::set_enabled(int id, bool enabled, std::string reason, std::string next_step) {
    const auto index = index_of(id);
    if (!index) return false;
    controls_[*index].enabled = enabled;
    controls_[*index].disabled_reason = enabled ? std::string{} : std::move(reason);
    controls_[*index].next_step = enabled ? std::string{} : std::move(next_step);
    repair_focus();
    return true;
}

bool UiTree::focus(int id) noexcept {
    const auto index = index_of(id);
    if (!index || !controls_[*index].visible || !controls_[*index].focusable) return false;
    focused_ = *index;
    return true;
}

std::optional<int> UiTree::focused_id() const noexcept {
    if (!focused_ || *focused_ >= controls_.size()) return std::nullopt;
    return controls_[*focused_].id;
}

std::optional<Control> UiTree::control(int id) const {
    const auto index = index_of(id);
    if (!index) return std::nullopt;
    return controls_[*index];
}

std::optional<std::size_t> UiTree::index_of(int id) const noexcept {
    for (std::size_t index = 0; index < controls_.size(); ++index) {
        if (controls_[index].id == id) return index;
    }
    return std::nullopt;
}

std::optional<std::size_t> UiTree::hit_test(int x, int y) const noexcept {
    for (std::size_t reverse = controls_.size(); reverse > 0; --reverse) {
        const std::size_t index = reverse - 1;
        const auto& item = controls_[index];
        if (item.visible && item.bounds.contains(x, y)) return index;
    }
    return std::nullopt;
}

void UiTree::repair_focus() noexcept {
    if (focused_ && *focused_ < controls_.size()) {
        const auto& current = controls_[*focused_];
        if (current.visible && current.focusable) return;
    }
    focused_.reset();
    for (std::size_t index = 0; index < controls_.size(); ++index) {
        if (controls_[index].visible && controls_[index].focusable) {
            focused_ = index;
            return;
        }
    }
}

bool UiTree::move_focus(int dx, int dy) noexcept {
    repair_focus();
    if (!focused_) return false;
    const auto& current = controls_[*focused_];
    const int current_x = current.bounds.x + current.bounds.width / 2;
    const int current_y = current.bounds.y + current.bounds.height / 2;
    std::optional<std::size_t> best;
    long best_score = std::numeric_limits<long>::max();

    for (std::size_t index = 0; index < controls_.size(); ++index) {
        if (index == *focused_) continue;
        const auto& candidate = controls_[index];
        if (!candidate.visible || !candidate.focusable) continue;
        const int candidate_x = candidate.bounds.x + candidate.bounds.width / 2;
        const int candidate_y = candidate.bounds.y + candidate.bounds.height / 2;
        const int delta_x = candidate_x - current_x;
        const int delta_y = candidate_y - current_y;
        if ((dx < 0 && delta_x >= 0) || (dx > 0 && delta_x <= 0) ||
            (dy < 0 && delta_y >= 0) || (dy > 0 && delta_y <= 0)) {
            continue;
        }
        const long primary = dx != 0 ? std::labs(delta_x) : std::labs(delta_y);
        const long secondary = dx != 0 ? std::labs(delta_y) : std::labs(delta_x);
        const long score = primary * 1000L + secondary;
        if (score < best_score || (score == best_score && candidate.id < controls_[*best].id)) {
            best = index;
            best_score = score;
        }
    }
    if (!best) return false;
    focused_ = *best;
    return true;
}

UiAction UiTree::activate(std::size_t index) const {
    const auto& item = controls_[index];
    if (item.enabled) return {UiActionType::Activate, item.id, 0, {}};
    std::string message = item.disabled_reason;
    if (!item.next_step.empty()) {
        if (!message.empty()) message += " ";
        message += item.next_step;
    }
    return {UiActionType::ShowDisabledReason, item.id, 0, std::move(message)};
}

std::optional<UiAction> UiTree::route(const InputFrame& input) {
    repair_focus();

    if (input.cancel) {
        captured_.reset();
        return UiAction{UiActionType::Cancel, -1, 0, {}};
    }

    if (input.touch_pressed) {
        captured_ = hit_test(input.touch_x, input.touch_y);
        press_x_ = input.touch_x;
        press_y_ = input.touch_y;
        if (captured_ && controls_[*captured_].focusable) focused_ = *captured_;
        return std::nullopt;
    }

    if (input.touch_held && captured_) {
        const int delta_x = input.touch_x - press_x_;
        const int delta_y = input.touch_y - press_y_;
        if (std::abs(delta_x) + std::abs(delta_y) >= 6) {
            const int value = std::abs(delta_y) >= std::abs(delta_x) ? delta_y : delta_x;
            return UiAction{UiActionType::Drag, controls_[*captured_].id, value, {}};
        }
        return std::nullopt;
    }

    if (input.touch_released) {
        const auto released = hit_test(input.touch_x, input.touch_y);
        const auto captured = captured_;
        captured_.reset();
        if (captured && released && *captured == *released) return activate(*captured);
        return std::nullopt;
    }

    if (input.up && move_focus(0, -1)) return UiAction{UiActionType::FocusChanged, *focused_id(), 0, {}};
    if (input.down && move_focus(0, 1)) return UiAction{UiActionType::FocusChanged, *focused_id(), 0, {}};
    if (input.left && move_focus(-1, 0)) return UiAction{UiActionType::FocusChanged, *focused_id(), 0, {}};
    if (input.right && move_focus(1, 0)) return UiAction{UiActionType::FocusChanged, *focused_id(), 0, {}};
    if (input.confirm && focused_) return activate(*focused_);
    return std::nullopt;
}

}  // namespace deep_shelter::ui
