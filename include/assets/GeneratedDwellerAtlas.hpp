#pragma once

#include <cstddef>
#include <cstdint>

namespace deep_shelter::assets {

constexpr std::size_t kGeneratedDwellerAtlasWidth = 256;
constexpr std::size_t kGeneratedDwellerAtlasHeight = 256;
constexpr std::size_t kGeneratedDwellerFrameWidth = 24;
constexpr std::size_t kGeneratedDwellerFrameHeight = 32;
constexpr std::size_t kGeneratedDwellerFramesPerAnimation = 4;
constexpr std::size_t kGeneratedDwellerAnimationCount = 3;
constexpr std::size_t kGeneratedDwellerArchetypeCount = 5;
constexpr std::size_t kGeneratedDwellerFrameCount =
    kGeneratedDwellerFramesPerAnimation * kGeneratedDwellerAnimationCount *
    kGeneratedDwellerArchetypeCount;
constexpr std::size_t kGeneratedDwellerAtlasColumns = 10;
constexpr std::size_t kGeneratedDwellerAtlasRows = 6;
constexpr std::size_t kGeneratedDwellerPixelCount =
    kGeneratedDwellerAtlasWidth * kGeneratedDwellerAtlasHeight;
constexpr std::size_t kGeneratedDwellerRuntimeBytes =
    kGeneratedDwellerPixelCount * sizeof(std::uint16_t);

enum class DwellerArchetype : std::uint8_t {
    Technician = 0,
    Gardener = 1,
    WaterOperator = 2,
    Mechanic = 3,
    Civilian = 4,
};

enum class DwellerAnimation : std::uint8_t {
    Idle = 0,
    Work = 1,
    Walk = 2,
};

struct DwellerAtlasRegion {
    std::uint16_t x = 0;
    std::uint16_t y = 0;
    std::uint16_t width = 0;
    std::uint16_t height = 0;
};

[[nodiscard]] DwellerArchetype dweller_archetype_for_room(int room_index) noexcept;
[[nodiscard]] DwellerArchetype safe_dweller_archetype(int raw_archetype) noexcept;
[[nodiscard]] DwellerAtlasRegion dweller_atlas_region(
    DwellerArchetype archetype,
    DwellerAnimation animation,
    std::size_t frame) noexcept;
[[nodiscard]] std::size_t dweller_animation_frame(
    std::uint32_t simulation_tick,
    DwellerAnimation animation,
    std::uint32_t phase) noexcept;

void decode_generated_dweller_atlas(std::uint16_t* output,
                                    std::size_t output_pixels) noexcept;
void decode_generated_dweller_atlas_tiled(std::uint16_t* output,
                                          std::size_t output_pixels) noexcept;

}  // namespace deep_shelter::assets
