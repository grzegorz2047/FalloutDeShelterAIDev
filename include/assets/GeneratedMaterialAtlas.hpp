#pragma once

#include <cstddef>
#include <cstdint>

namespace deep_shelter::assets {

constexpr std::size_t kGeneratedMaterialAtlasWidth = 64;
constexpr std::size_t kGeneratedMaterialAtlasHeight = 16;
constexpr std::size_t kGeneratedMaterialTileSize = 16;
constexpr std::size_t kGeneratedMaterialPaletteEntries = 16;
constexpr std::size_t kGeneratedMaterialPixelCount =
    kGeneratedMaterialAtlasWidth * kGeneratedMaterialAtlasHeight;
constexpr std::size_t kGeneratedMaterialPackedBytes =
    kGeneratedMaterialPixelCount / 2;
constexpr std::size_t kGeneratedMaterialRuntimeBytes =
    kGeneratedMaterialPixelCount * sizeof(std::uint16_t);

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

// Row-major output, useful for host validation and previews.
void decode_generated_material_atlas(std::uint16_t* output,
                                     std::size_t output_pixels) noexcept;

// PICA200 8x8 Morton-tiled output, ready for C3D_TexUpload/C3D_Tex storage.
void decode_generated_material_atlas_tiled(std::uint16_t* output,
                                           std::size_t output_pixels) noexcept;

}  // namespace deep_shelter::assets
