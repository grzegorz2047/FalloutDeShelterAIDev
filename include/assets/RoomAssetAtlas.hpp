#pragma once

#include <cstddef>
#include <cstdint>

namespace deep_shelter::assets {

constexpr std::size_t kRoomAssetAtlasWidth = 512;
constexpr std::size_t kRoomAssetAtlasHeight = 256;
constexpr std::size_t kRoomAssetPixelCount =
    kRoomAssetAtlasWidth * kRoomAssetAtlasHeight;
constexpr std::size_t kRoomAssetRuntimeBytes =
    kRoomAssetPixelCount * sizeof(std::uint16_t);
constexpr std::size_t kRoomMaterialTileSize = 64;
constexpr std::size_t kRoomMaterialTileCount = 8;
constexpr std::size_t kRoomPropCellWidth = 128;
constexpr std::size_t kRoomPropCellHeight = 64;
constexpr std::size_t kRoomPropCount = 12;

enum class RoomProp : std::uint8_t {
    ControlConsole = 0,
    PowerGenerator = 1,
    WaterMachinery = 2,
    StorageShelf = 3,
    Lockers = 4,
    HydroponicPlanter = 5,
    Terminal = 6,
    BunkBeds = 7,
    WorkTable = 8,
    StorageCrate = 9,
    Sofa = 10,
    CeilingLight = 11,
};

struct RoomAtlasRegion {
    std::uint16_t x = 0;
    std::uint16_t y = 0;
    std::uint16_t width = 0;
    std::uint16_t height = 0;
};

[[nodiscard]] constexpr RoomAtlasRegion room_prop_region(
    RoomProp prop) noexcept {
    const std::size_t index = static_cast<std::size_t>(prop);
    return {
        static_cast<std::uint16_t>((index % 4u) * kRoomPropCellWidth),
        static_cast<std::uint16_t>(
            kRoomMaterialTileSize + (index / 4u) * kRoomPropCellHeight),
        static_cast<std::uint16_t>(kRoomPropCellWidth),
        static_cast<std::uint16_t>(kRoomPropCellHeight),
    };
}

}  // namespace deep_shelter::assets
