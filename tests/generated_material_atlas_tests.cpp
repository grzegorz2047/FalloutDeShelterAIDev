#include "assets/GeneratedMaterialAtlas.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

using namespace deep_shelter::assets;

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

}  // namespace

int main() {
    static_assert(kGeneratedMaterialAtlasWidth == 64);
    static_assert(kGeneratedMaterialAtlasHeight == 16);
    static_assert(kGeneratedMaterialTileSize == 16);
    static_assert(kGeneratedMaterialPackedBytes == 512);
    static_assert(kGeneratedMaterialRuntimeBytes == 2048);

    std::array<std::uint16_t, kGeneratedMaterialPixelCount> linear{};
    std::array<std::uint16_t, kGeneratedMaterialPixelCount> tiled{};

    decode_generated_material_atlas(linear.data(), linear.size());
    decode_generated_material_atlas_tiled(tiled.data(), tiled.size());

    for (std::size_t y = 0; y < kGeneratedMaterialAtlasHeight; ++y) {
        for (std::size_t x = 0; x < kGeneratedMaterialAtlasWidth; ++x) {
            const std::size_t source = y * kGeneratedMaterialAtlasWidth + x;
            assert(linear[source] == tiled[pica_tile_offset(x, y)]);
        }
    }

    std::array<std::uint16_t, 8> too_small{};
    too_small.fill(0x55aau);
    decode_generated_material_atlas(too_small.data(), too_small.size());
    decode_generated_material_atlas_tiled(too_small.data(), too_small.size());
    for (const auto pixel : too_small) assert(pixel == 0x55aau);

    return 0;
}
