#pragma once

#include <array>
#include <cstddef>
#include <cstdio>

#include <citro2d.h>

namespace deep_shelter::ui::embedded_text {

using Glyph = std::array<u8, 7>;

[[nodiscard]] constexpr Glyph glyph(char raw) noexcept {
    const char c = raw >= 'a' && raw <= 'z' ? static_cast<char>(raw - 'a' + 'A') : raw;
    switch (c) {
        case 'A': return {14, 17, 17, 31, 17, 17, 17};
        case 'B': return {30, 17, 17, 30, 17, 17, 30};
        case 'C': return {14, 17, 16, 16, 16, 17, 14};
        case 'D': return {30, 17, 17, 17, 17, 17, 30};
        case 'E': return {31, 16, 16, 30, 16, 16, 31};
        case 'F': return {31, 16, 16, 30, 16, 16, 16};
        case 'G': return {14, 17, 16, 23, 17, 17, 14};
        case 'H': return {17, 17, 17, 31, 17, 17, 17};
        case 'I': return {31, 4, 4, 4, 4, 4, 31};
        case 'J': return {7, 2, 2, 2, 18, 18, 12};
        case 'K': return {17, 18, 20, 24, 20, 18, 17};
        case 'L': return {16, 16, 16, 16, 16, 16, 31};
        case 'M': return {17, 27, 21, 21, 17, 17, 17};
        case 'N': return {17, 25, 21, 19, 17, 17, 17};
        case 'O': return {14, 17, 17, 17, 17, 17, 14};
        case 'P': return {30, 17, 17, 30, 16, 16, 16};
        case 'Q': return {14, 17, 17, 17, 21, 18, 13};
        case 'R': return {30, 17, 17, 30, 20, 18, 17};
        case 'S': return {15, 16, 16, 14, 1, 1, 30};
        case 'T': return {31, 4, 4, 4, 4, 4, 4};
        case 'U': return {17, 17, 17, 17, 17, 17, 14};
        case 'V': return {17, 17, 17, 17, 17, 10, 4};
        case 'W': return {17, 17, 17, 21, 21, 21, 10};
        case 'X': return {17, 17, 10, 4, 10, 17, 17};
        case 'Y': return {17, 17, 10, 4, 4, 4, 4};
        case 'Z': return {31, 1, 2, 4, 8, 16, 31};
        case '0': return {14, 17, 19, 21, 25, 17, 14};
        case '1': return {4, 12, 4, 4, 4, 4, 14};
        case '2': return {14, 17, 1, 2, 4, 8, 31};
        case '3': return {30, 1, 1, 14, 1, 1, 30};
        case '4': return {2, 6, 10, 18, 31, 2, 2};
        case '5': return {31, 16, 16, 30, 1, 1, 30};
        case '6': return {14, 16, 16, 30, 17, 17, 14};
        case '7': return {31, 1, 2, 4, 8, 8, 8};
        case '8': return {14, 17, 17, 14, 17, 17, 14};
        case '9': return {14, 17, 17, 15, 1, 1, 14};
        case ':': return {0, 4, 4, 0, 4, 4, 0};
        case '.': return {0, 0, 0, 0, 0, 4, 4};
        case ',': return {0, 0, 0, 0, 4, 4, 8};
        case '-': return {0, 0, 0, 31, 0, 0, 0};
        case '/': return {1, 2, 2, 4, 8, 8, 16};
        case '?': return {14, 17, 1, 2, 4, 0, 4};
        case '!': return {4, 4, 4, 4, 4, 0, 4};
        case '+': return {0, 4, 4, 31, 4, 4, 0};
        default: return {0, 0, 0, 0, 0, 0, 0};
    }
}

[[nodiscard]] inline std::array<char, 256>& text_buffer() noexcept {
    static std::array<char, 256> value{};
    return value;
}

inline std::size_t parse(C2D_Text*, C2D_TextBuf, const char* value) noexcept {
    auto& buffer = text_buffer();
    std::snprintf(buffer.data(), buffer.size(), "%s", value != nullptr ? value : "");
    return 0;
}

inline void optimize(C2D_Text*) noexcept {}

inline void draw_string(const char* value,
                        float x,
                        float y,
                        float z,
                        float scale_x,
                        float scale_y,
                        u32 color) noexcept {
    if (value == nullptr) return;

    const float pixel_width = scale_x * 2.05f;
    const float pixel_height = scale_y * 3.35f;
    const float advance = pixel_width * 6.0f;
    const float line_height = pixel_height * 8.0f;
    const float origin_x = x;
    constexpr float max_x = 316.0f;

    for (const char* cursor = value; *cursor != '\0'; ++cursor) {
        const char c = *cursor;
        if (c == '\n') {
            x = origin_x;
            y += line_height;
            continue;
        }
        if (c == ' ') {
            if (x + advance > max_x) {
                x = origin_x;
                y += line_height;
            } else {
                x += advance;
            }
            continue;
        }
        if (x + advance > max_x) {
            x = origin_x;
            y += line_height;
        }

        const Glyph rows = glyph(c);
        for (int row = 0; row < 7; ++row) {
            for (int column = 0; column < 5; ++column) {
                if ((rows[static_cast<std::size_t>(row)] & (1u << (4 - column))) != 0) {
                    C2D_DrawRectSolid(x + static_cast<float>(column) * pixel_width,
                                      y + static_cast<float>(row) * pixel_height,
                                      z,
                                      pixel_width,
                                      pixel_height,
                                      color);
                }
            }
        }
        x += advance;
    }
}

inline void draw(C2D_Text*,
                 u32,
                 float x,
                 float y,
                 float z,
                 float scale_x,
                 float scale_y,
                 u32 color) noexcept {
    draw_string(text_buffer().data(), x, y, z, scale_x, scale_y, color);
}

}  // namespace deep_shelter::ui::embedded_text

#define C2D_TextParse(text, buffer, value) \
    ::deep_shelter::ui::embedded_text::parse((text), (buffer), (value))
#define C2D_TextOptimize(text) \
    ::deep_shelter::ui::embedded_text::optimize((text))
#define C2D_DrawText(text, flags, x, y, z, scale_x, scale_y, color) \
    ::deep_shelter::ui::embedded_text::draw( \
        (text), (flags), (x), (y), (z), (scale_x), (scale_y), (color))
