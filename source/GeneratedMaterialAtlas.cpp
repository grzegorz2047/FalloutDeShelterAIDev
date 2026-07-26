#include "assets/GeneratedMaterialAtlas.hpp"

namespace deep_shelter::assets {
namespace {

constexpr std::size_t pica_tile_offset(std::size_t x, std::size_t y) noexcept {
    const std::size_t tile_x = x / 8;
    const std::size_t tile_y = y / 8;
    const std::size_t local_x = x & 7u;
    const std::size_t local_y = y & 7u;
    const std::size_t tile_index =
        tile_y * (kGeneratedMaterialAtlasWidth / 8) + tile_x;
    const std::size_t morton =
        (local_x & 1u) |
        ((local_y & 1u) << 1u) |
        ((local_x & 2u) << 1u) |
        ((local_y & 2u) << 2u) |
        ((local_x & 4u) << 2u) |
        ((local_y & 4u) << 3u);
    return tile_index * 64u + morton;
}

constexpr std::uint8_t clamp_index(int value) noexcept {
    return static_cast<std::uint8_t>(value < 0 ? 0 : (value > 15 ? 15 : value));
}

constexpr std::uint8_t material_index(std::size_t x, std::size_t y) noexcept {
    const std::size_t tile = x / kGeneratedMaterialTileSize;
    const int lx = static_cast<int>(x % kGeneratedMaterialTileSize);
    const int ly = static_cast<int>(y);
    switch (tile) {
        case 0: {
            const int noise = (lx * 11 + ly * 7 + (lx * ly) * 3) & 7;
            return clamp_index(3 + noise + (((lx + ly) % 7) == 0 ? 3 : 0));
        }
        case 1: {
            const int band = ((lx + ly * 2) / 4) & 3;
            const int chips = ((lx * 5 + ly * 3) % 11) == 0 ? 3 : 0;
            return clamp_index(5 + band + chips);
        }
        case 2: {
            const bool seam = lx == 0 || ly == 0 || lx == 15 || ly == 15 || lx == 8;
            const bool rivet = ((lx == 2 || lx == 13) && (ly == 2 || ly == 13));
            return rivet ? 14 : (seam ? 4 : 9 + ((lx + ly) & 1));
        }
        case 3: {
            const bool frame = lx < 2 || lx > 13 || ly < 2 || ly > 13;
            const bool inset = lx > 4 && lx < 11 && ly > 4 && ly < 11;
            return frame ? 5 : (inset ? 11 : 8);
        }
        case 4: {
            const bool bar = (lx % 4) == 0 || (ly % 4) == 0;
            return bar ? 12 : (((lx + ly) & 1) ? 3 : 5);
        }
        case 5: {
            const int wave = ((lx + (ly & 1) * 2) % 6) < 3 ? 2 : 0;
            return clamp_index(7 + wave + (ly == 3 || ly == 11 ? 4 : 0));
        }
        case 6: {
            if (ly > 11) return 5 + ((lx / 2) & 1);
            const int stem = (lx % 5) == 2 ? 11 : 7;
            const bool leaf = ((lx + ly) % 5) == 0 || ((lx - ly + 16) % 7) == 0;
            return leaf ? 13 : stem;
        }
        default: {
            const bool frame = lx < 2 || lx > 13 || ly < 2 || ly > 13;
            const bool screen = lx >= 6 && lx <= 12 && ly >= 3 && ly <= 8;
            const bool lamp = lx >= 2 && lx <= 4 && (ly == 4 || ly == 8 || ly == 12);
            return frame ? 3 : (screen ? 14 : (lamp ? 15 : 6));
        }
    }
}

std::uint16_t decoded_pixel(std::size_t pixel_index) noexcept {
    const std::size_t x = pixel_index % kGeneratedMaterialAtlasWidth;
    const std::size_t y = pixel_index / kGeneratedMaterialAtlasWidth;
    return kGeneratedMaterialPaletteRgb565[material_index(x, y)];
}

}  // namespace

alignas(16) const std::uint16_t
    kGeneratedMaterialPaletteRgb565[kGeneratedMaterialPaletteEntries] = {
        0x2104, 0x2965, 0x39c7, 0x4228, 0x528a, 0x5aeb, 0x6b4d, 0x73ae,
        0x8410, 0x8c71, 0x9cf3, 0xad55, 0xbdd7, 0xce59, 0xdefb, 0xf7be,
};

void decode_generated_material_atlas(std::uint16_t* output,
                                     std::size_t output_pixels) noexcept {
    if (output == nullptr || output_pixels < kGeneratedMaterialPixelCount) return;
    for (std::size_t pixel = 0; pixel < kGeneratedMaterialPixelCount; ++pixel) {
        output[pixel] = decoded_pixel(pixel);
    }
}

void decode_generated_material_atlas_tiled(std::uint16_t* output,
                                           std::size_t output_pixels) noexcept {
    if (output == nullptr || output_pixels < kGeneratedMaterialPixelCount) return;
    for (std::size_t y = 0; y < kGeneratedMaterialAtlasHeight; ++y) {
        for (std::size_t x = 0; x < kGeneratedMaterialAtlasWidth; ++x) {
            const std::size_t source = y * kGeneratedMaterialAtlasWidth + x;
            output[pica_tile_offset(x, y)] = decoded_pixel(source);
        }
    }
}

}  // namespace deep_shelter::assets
