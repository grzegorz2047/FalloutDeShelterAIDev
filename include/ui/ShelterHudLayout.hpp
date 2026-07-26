#pragma once

#include "ui/UiFramework.hpp"

namespace deep_shelter::ui::shelter_hud {

constexpr int kPrimaryActionId = 1;
constexpr int kBuildActionId = 2;
constexpr int kSaveActionId = 3;

constexpr Rect kPrimaryActionBounds{10, 160, 194, 68};
constexpr Rect kBuildActionBounds{212, 160, 98, 32};
constexpr Rect kSaveActionBounds{212, 196, 98, 32};

[[nodiscard]] constexpr bool inside_bottom_screen(Rect bounds) noexcept {
    return bounds.x >= 0 && bounds.y >= 0 &&
           bounds.x + bounds.width <= 320 &&
           bounds.y + bounds.height <= 240;
}

[[nodiscard]] constexpr bool touch_target_is_large_enough(
    Rect bounds) noexcept {
    return bounds.width >= 44 && bounds.height >= 32;
}

static_assert(inside_bottom_screen(kPrimaryActionBounds));
static_assert(inside_bottom_screen(kBuildActionBounds));
static_assert(inside_bottom_screen(kSaveActionBounds));
static_assert(touch_target_is_large_enough(kPrimaryActionBounds));
static_assert(touch_target_is_large_enough(kBuildActionBounds));
static_assert(touch_target_is_large_enough(kSaveActionBounds));

}  // namespace deep_shelter::ui::shelter_hud
