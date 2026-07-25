#pragma once

#include <cstddef>
#include <cstdint>

namespace deep_shelter::assets {

constexpr std::size_t kGeneratedUiAtlasWidth = 128;
constexpr std::size_t kGeneratedUiAtlasHeight = 64;
constexpr std::size_t kGeneratedUiAtlasPixelCount =
    kGeneratedUiAtlasWidth * kGeneratedUiAtlasHeight;
constexpr std::size_t kGeneratedUiAtlasRuntimeBytes =
    kGeneratedUiAtlasPixelCount * sizeof(std::uint32_t);
constexpr std::size_t kGeneratedUiIconSize = 16;
constexpr std::size_t kGeneratedUiButtonWidth = 32;
constexpr std::size_t kGeneratedUiButtonHeight = 24;

enum class UiIcon : std::uint8_t {
    Build = 0,
    Work = 1,
    Collect = 2,
    Save = 3,
    Power = 4,
    Food = 5,
    Water = 6,
    Credits = 7,
    Count = 8,
};

enum class UiButtonState : std::uint8_t {
    Normal = 0,
    Focused = 1,
    Pressed = 2,
    Disabled = 3,
    Count = 4,
};

struct UiAtlasRegion {
    std::uint16_t x = 0;
    std::uint16_t y = 0;
    std::uint16_t width = 0;
    std::uint16_t height = 0;
};

[[nodiscard]] constexpr UiAtlasRegion ui_icon_region(UiIcon icon) noexcept {
    return {static_cast<std::uint16_t>(static_cast<std::uint8_t>(icon) *
                                      kGeneratedUiIconSize),
            0,
            static_cast<std::uint16_t>(kGeneratedUiIconSize),
            static_cast<std::uint16_t>(kGeneratedUiIconSize)};
}

[[nodiscard]] constexpr UiAtlasRegion ui_button_region(UiButtonState state) noexcept {
    return {static_cast<std::uint16_t>(static_cast<std::uint8_t>(state) *
                                      kGeneratedUiButtonWidth),
            static_cast<std::uint16_t>(kGeneratedUiIconSize),
            static_cast<std::uint16_t>(kGeneratedUiButtonWidth),
            static_cast<std::uint16_t>(kGeneratedUiButtonHeight)};
}

// RGBA8 pixels in conventional top-left row-major order for host tests/previews.
void decode_generated_ui_atlas(std::uint32_t* output,
                               std::size_t output_pixels) noexcept;

// RGBA8 pixels in the PICA200 8x8 Morton-tiled order expected by C3D_Tex.
void decode_generated_ui_atlas_tiled(std::uint32_t* output,
                                     std::size_t output_pixels) noexcept;

}  // namespace deep_shelter::assets
