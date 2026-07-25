#pragma once

#include <cstddef>
#include <cstdint>

namespace deep_shelter::assets {

constexpr std::size_t kGeneratedMaterialAtlasWidth = 128;
constexpr std::size_t kGeneratedMaterialAtlasHeight = 32;
constexpr std::size_t kGeneratedMaterialTileSize = 32;
constexpr std::size_t kGeneratedMaterialAtlasBytes =
    kGeneratedMaterialAtlasWidth * kGeneratedMaterialAtlasHeight * sizeof(std::uint16_t);

enum class GeneratedMaterial : std::uint8_t {
    Rock = 0,
    Steel = 1,
    Grating = 2,
    ControlPanel = 3,
};

extern const std::uint16_t kGeneratedMaterialAtlasRgb565[
    kGeneratedMaterialAtlasWidth * kGeneratedMaterialAtlasHeight];

}  // namespace deep_shelter::assets
