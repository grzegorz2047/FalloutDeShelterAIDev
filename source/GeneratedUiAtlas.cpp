#include "assets/GeneratedUiAtlas.hpp"

#include <algorithm>
#include <cstdlib>

namespace deep_shelter::assets {
namespace {

// Startup-only scratch storage. Keeping this outside the main-thread stack avoids
// the same class of ARM11 stack overflow previously fixed in the scene renderer.
alignas(16) std::uint32_t ui_atlas_scratch[kGeneratedUiAtlasPixelCount];

constexpr std::uint32_t rgba(std::uint8_t r,
                             std::uint8_t g,
                             std::uint8_t b,
                             std::uint8_t a = 255) noexcept {
    return static_cast<std::uint32_t>(r) |
           (static_cast<std::uint32_t>(g) << 8u) |
           (static_cast<std::uint32_t>(b) << 16u) |
           (static_cast<std::uint32_t>(a) << 24u);
}

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

void put(std::uint32_t* pixels,
         int x,
         int y,
         std::uint32_t color) noexcept {
    if (x < 0 || y < 0 || x >= static_cast<int>(kGeneratedUiAtlasWidth) ||
        y >= static_cast<int>(kGeneratedUiAtlasHeight)) {
        return;
    }
    pixels[static_cast<std::size_t>(y) * kGeneratedUiAtlasWidth +
           static_cast<std::size_t>(x)] = color;
}

void rect(std::uint32_t* pixels,
          int x,
          int y,
          int width,
          int height,
          std::uint32_t color) noexcept {
    for (int py = y; py < y + height; ++py) {
        for (int px = x; px < x + width; ++px) put(pixels, px, py, color);
    }
}

void line(std::uint32_t* pixels,
          int x0,
          int y0,
          int x1,
          int y1,
          std::uint32_t color) noexcept {
    const int dx = std::abs(x1 - x0);
    const int sx = x0 < x1 ? 1 : -1;
    const int dy = -std::abs(y1 - y0);
    const int sy = y0 < y1 ? 1 : -1;
    int error = dx + dy;
    while (true) {
        put(pixels, x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        const int twice = error * 2;
        if (twice >= dy) {
            error += dy;
            x0 += sx;
        }
        if (twice <= dx) {
            error += dx;
            y0 += sy;
        }
    }
}

void draw_icon(std::uint32_t* pixels, UiIcon icon) noexcept {
    const int ox = static_cast<int>(static_cast<std::uint8_t>(icon)) * 16;
    constexpr std::uint32_t clear = rgba(0, 0, 0, 0);
    constexpr std::uint32_t ink = rgba(246, 228, 151);
    constexpr std::uint32_t accent = rgba(106, 221, 184);
    rect(pixels, ox, 0, 16, 16, clear);

    switch (icon) {
        case UiIcon::Build:
            line(pixels, ox + 3, 12, ox + 12, 3, ink);
            line(pixels, ox + 2, 4, ox + 5, 1, ink);
            line(pixels, ox + 3, 5, ox + 6, 2, ink);
            rect(pixels, ox + 9, 8, 4, 4, accent);
            break;
        case UiIcon::Work:
            rect(pixels, ox + 4, 3, 8, 3, ink);
            rect(pixels, ox + 2, 6, 12, 7, accent);
            rect(pixels, ox + 6, 8, 4, 2, ink);
            break;
        case UiIcon::Collect:
            line(pixels, ox + 8, 2, ox + 8, 10, ink);
            line(pixels, ox + 4, 7, ox + 8, 11, ink);
            line(pixels, ox + 12, 7, ox + 8, 11, ink);
            rect(pixels, ox + 3, 12, 10, 2, accent);
            break;
        case UiIcon::Save:
            rect(pixels, ox + 3, 2, 10, 12, ink);
            rect(pixels, ox + 5, 3, 6, 4, rgba(43, 75, 82));
            rect(pixels, ox + 5, 9, 6, 4, accent);
            break;
        case UiIcon::Power:
            line(pixels, ox + 9, 1, ox + 4, 9, ink);
            line(pixels, ox + 4, 9, ox + 8, 9, ink);
            line(pixels, ox + 8, 9, ox + 6, 15, accent);
            line(pixels, ox + 6, 15, ox + 12, 7, accent);
            break;
        case UiIcon::Food:
            rect(pixels, ox + 3, 3, 10, 2, ink);
            rect(pixels, ox + 4, 5, 8, 8, accent);
            line(pixels, ox + 5, 2, ox + 11, 2, ink);
            break;
        case UiIcon::Water:
            line(pixels, ox + 8, 1, ox + 3, 9, ink);
            line(pixels, ox + 8, 1, ox + 13, 9, ink);
            line(pixels, ox + 3, 9, ox + 8, 14, accent);
            line(pixels, ox + 13, 9, ox + 8, 14, accent);
            break;
        case UiIcon::Credits:
            rect(pixels, ox + 3, 3, 10, 10, ink);
            rect(pixels, ox + 5, 5, 6, 6, rgba(43, 75, 82));
            rect(pixels, ox + 7, 4, 2, 8, accent);
            break;
        case UiIcon::Count:
            break;
    }
}

void draw_button(std::uint32_t* pixels, UiButtonState state) noexcept {
    const int ox = static_cast<int>(static_cast<std::uint8_t>(state)) * 32;
    const int oy = 16;
    std::uint32_t fill = rgba(43, 83, 91);
    std::uint32_t border = rgba(128, 185, 188);
    if (state == UiButtonState::Focused) {
        fill = rgba(109, 91, 42);
        border = rgba(250, 207, 91);
    } else if (state == UiButtonState::Pressed) {
        fill = rgba(32, 125, 111);
        border = rgba(138, 246, 205);
    } else if (state == UiButtonState::Disabled) {
        fill = rgba(52, 57, 59);
        border = rgba(91, 98, 100);
    }
    rect(pixels, ox, oy, 32, 24, rgba(0, 0, 0, 0));
    rect(pixels, ox + 1, oy + 1, 30, 22, border);
    rect(pixels, ox + 3, oy + 3, 26, 18, fill);
    line(pixels, ox + 4, oy + 4, ox + 27, oy + 4, rgba(255, 255, 255, 60));
    line(pixels, ox + 4, oy + 20, ox + 27, oy + 20, rgba(0, 0, 0, 90));
}

void render_linear(std::uint32_t* output) noexcept {
    std::fill(output, output + kGeneratedUiAtlasPixelCount, rgba(0, 0, 0, 0));
    for (std::uint8_t icon = 0; icon < static_cast<std::uint8_t>(UiIcon::Count); ++icon) {
        draw_icon(output, static_cast<UiIcon>(icon));
    }
    for (std::uint8_t state = 0;
         state < static_cast<std::uint8_t>(UiButtonState::Count);
         ++state) {
        draw_button(output, static_cast<UiButtonState>(state));
    }
}

}  // namespace

void decode_generated_ui_atlas(std::uint32_t* output,
                               std::size_t output_pixels) noexcept {
    if (output == nullptr || output_pixels < kGeneratedUiAtlasPixelCount) return;
    render_linear(output);
}

void decode_generated_ui_atlas_tiled(std::uint32_t* output,
                                     std::size_t output_pixels) noexcept {
    if (output == nullptr || output_pixels < kGeneratedUiAtlasPixelCount) return;
    render_linear(ui_atlas_scratch);
    for (std::size_t y = 0; y < kGeneratedUiAtlasHeight; ++y) {
        for (std::size_t x = 0; x < kGeneratedUiAtlasWidth; ++x) {
            output[pica_tile_offset(x, y)] =
                ui_atlas_scratch[y * kGeneratedUiAtlasWidth + x];
        }
    }
}

}  // namespace deep_shelter::assets
