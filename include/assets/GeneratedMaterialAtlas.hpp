#pragma once

#include <cstddef>
#include <cstdint>

namespace deep_shelter::assets {

constexpr std::size_t kGeneratedMaterialAtlasWidth = 64;
constexpr std::size_t kGeneratedMaterialAtlasHeight = 16;
constexpr std::size_t kGeneratedMaterialTileSize = 16;
constexpr std::size_t kGeneratedMaterialPaletteEntries = 16;
constexpr std::size_t kGeneratedMaterialPackedBytes =
    kGeneratedMaterialAtlasWidth * kGeneratedMaterialAtlasHeight / 2;
constexpr std::size_t kGeneratedMaterialRuntimeBytes =
    kGeneratedMaterialAtlasWidth * kGeneratedMaterialAtlasHeight * sizeof(std::uint16_t);

enum class GeneratedMaterial : std::uint8_t {
    Rock = 0,
    Steel = 1,
    Grating = 2,
    ControlPanel = 3,
};

extern const std::uint16_t
    kGeneratedMaterialPaletteRgb565[kGeneratedMaterialPaletteEntries];
extern const std::uint8_t
    kGeneratedMaterialIndices4bpp[kGeneratedMaterialPackedBytes];

void decode_generated_material_atlas(std::uint16_t* output,
                                     std::size_t output_pixels) noexcept;

}  // namespace deep_shelter::assets
