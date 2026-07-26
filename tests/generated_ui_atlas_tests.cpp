#include "assets/GeneratedUiAtlas.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

using namespace deep_shelter::assets;

namespace {

constexpr std::size_t pica_tile_offset(std::size_t x, std::size_t y) noexcept {
    const std::size_t tile_x = x / 8u;
    const std::size_t tile_y = y / 8u;
    const std::size_t local_x = x & 7u;
    const std::size_t local_y = y & 7u;
    const std::size_t tile_index =
        tile_y * (kGeneratedUiAtlasWidth / 8u) + tile_x;
    const std::size_t morton =
        (local_x & 1u) |
        ((local_y & 1u) << 1u) |
        ((local_x & 2u) << 1u) |
        ((local_y & 2u) << 2u) |
        ((local_x & 4u) << 2u) |
        ((local_y & 4u) << 3u);
    return tile_index * 64u + morton;
}

std::uint8_t alpha(std::uint32_t pixel) noexcept {
    return static_cast<std::uint8_t>(pixel >> 24u);
}

constexpr std::uint32_t pica_rgba8(std::uint32_t color) noexcept {
    const std::uint32_t r = color & 0xffu;
    const std::uint32_t g = (color >> 8u) & 0xffu;
    const std::uint32_t b = (color >> 16u) & 0xffu;
    const std::uint32_t a = (color >> 24u) & 0xffu;
    return a | (b << 8u) | (g << 16u) | (r << 24u);
}

}  // namespace

int main() {
    static_assert(kGeneratedUiAtlasWidth == 128);
    static_assert(kGeneratedUiAtlasHeight == 64);
    static_assert(kGeneratedUiAtlasRuntimeBytes == 32768);

    std::array<std::uint32_t, kGeneratedUiAtlasPixelCount> linear{};
    std::array<std::uint32_t, kGeneratedUiAtlasPixelCount> tiled{};
    decode_generated_ui_atlas(linear.data(), linear.size());
    decode_generated_ui_atlas_tiled(tiled.data(), tiled.size());

    for (std::size_t y = 0; y < kGeneratedUiAtlasHeight; ++y) {
        for (std::size_t x = 0; x < kGeneratedUiAtlasWidth; ++x) {
            const std::size_t source = y * kGeneratedUiAtlasWidth + x;
            assert(pica_rgba8(linear[source]) ==
                   tiled[pica_tile_offset(x, y)]);
        }
    }

    // Guard the GPU byte order. The previous row-major copy displayed the
    // intended cream/mint palette as magenta in Azahar.
    const auto work = ui_icon_region(UiIcon::Work);
    const auto work_accent =
        linear[(work.y + 7u) * kGeneratedUiAtlasWidth + work.x + 3u];
    assert(work_accent == 0xffb8dd6au);
    assert(pica_rgba8(work_accent) == 0x6addb8ffu);

    for (std::uint8_t index = 0; index < static_cast<std::uint8_t>(UiIcon::Count); ++index) {
        const auto region = ui_icon_region(static_cast<UiIcon>(index));
        std::size_t opaque = 0;
        for (std::size_t y = region.y; y < region.y + region.height; ++y) {
            for (std::size_t x = region.x; x < region.x + region.width; ++x) {
                if (alpha(linear[y * kGeneratedUiAtlasWidth + x]) != 0) ++opaque;
            }
        }
        assert(opaque >= 8); // every icon must remain visible after simplification
    }

    for (std::uint8_t index = 0;
         index < static_cast<std::uint8_t>(UiButtonState::Count);
         ++index) {
        const auto region = ui_button_region(static_cast<UiButtonState>(index));
        const auto center = linear[(region.y + region.height / 2u) * kGeneratedUiAtlasWidth +
                                   region.x + region.width / 2u];
        assert(alpha(center) == 255);
    }

    const auto normal = ui_button_region(UiButtonState::Normal);
    const auto focused = ui_button_region(UiButtonState::Focused);
    const auto pressed = ui_button_region(UiButtonState::Pressed);
    const auto disabled = ui_button_region(UiButtonState::Disabled);
    const auto sample = [](const auto& pixels, UiAtlasRegion region) {
        return pixels[(region.y + region.height / 2u) * kGeneratedUiAtlasWidth +
                      region.x + region.width / 2u];
    };
    assert(sample(linear, normal) != sample(linear, focused));
    assert(sample(linear, focused) != sample(linear, pressed));
    assert(sample(linear, pressed) != sample(linear, disabled));

    std::array<std::uint32_t, 8> too_small{};
    too_small.fill(0x11223344u);
    decode_generated_ui_atlas(too_small.data(), too_small.size());
    decode_generated_ui_atlas_tiled(too_small.data(), too_small.size());
    for (const auto pixel : too_small) assert(pixel == 0x11223344u);

    return 0;
}
