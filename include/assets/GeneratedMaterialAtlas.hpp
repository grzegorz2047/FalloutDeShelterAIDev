#pragma once

#include <cstddef>
#include <cstdint>

namespace deep_shelter::assets {

constexpr std::size_t kGeneratedMaterialAtlasWidth = 128;
constexpr std::size_t kGeneratedMaterialAtlasHeight = 16;
constexpr std::size_t kGeneratedMaterialTileSize = 16;
constexpr std::size_t kGeneratedMaterialTileCount = 8;
constexpr std::size_t kGeneratedMaterialPaletteEntries = 16;
constexpr std::size_t kGeneratedMaterialPixelCount =
    kGeneratedMaterialAtlasWidth * kGeneratedMaterialAtlasHeight;
constexpr std::size_t kGeneratedMaterialPackedBytes =
    kGeneratedMaterialPixelCount / 2;
constexpr std::size_t kGeneratedMaterialRuntimeBytes =
    kGeneratedMaterialPixelCount * sizeof(std::uint16_t);

// Compact 16x16 materials derived from the reference pack attached to issue #85.
// The production build stores only this generated 4bpp atlas, never loose PNGs.
enum class GeneratedMaterial : std::uint8_t {
    Rock = 0,
    ExcavatedRock = 1,
    Steel = 2,
    VaultPanel = 3,
    Grating = 4,
    Water = 5,
    Hydroponic = 6,
    ControlPanel = 7,
};

extern const std::uint16_t
    kGeneratedMaterialPaletteRgb565[kGeneratedMaterialPaletteEntries];
extern const std::uint8_t
    kGeneratedMaterialIndices4bpp[kGeneratedMaterialPackedBytes];

void decode_generated_material_atlas(std::uint16_t* output,
                                     std::size_t output_pixels) noexcept;
void decode_generated_material_atlas_tiled(std::uint16_t* output,
                                           std::size_t output_pixels) noexcept;

}  // namespace deep_shelter::assets
