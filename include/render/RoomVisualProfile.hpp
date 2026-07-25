#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace deep_shelter::render {

enum class DominantProp : std::uint8_t {
    Generator = 0,
    Planter = 1,
    WaterTank = 2,
    Workbench = 3,
    StorageRack = 4,
    BunkBed = 5,
};

struct RoomVisualProfile {
    DominantProp dominant_prop;
    std::uint8_t secondary_prop_count;
    float dominant_width;
    float dominant_height;
    float resident_clearance_width;
    bool uses_translucent_glow;
};

constexpr std::array<RoomVisualProfile, 6> kRoomVisualProfiles{{
    {DominantProp::Generator, 3, 58.0f, 42.0f, 31.0f, true},
    {DominantProp::Planter, 4, 104.0f, 28.0f, 24.0f, true},
    {DominantProp::WaterTank, 3, 72.0f, 43.0f, 26.0f, true},
    {DominantProp::Workbench, 4, 92.0f, 31.0f, 28.0f, true},
    {DominantProp::StorageRack, 5, 83.0f, 45.0f, 22.0f, false},
    {DominantProp::BunkBed, 4, 76.0f, 36.0f, 30.0f, true},
}};

[[nodiscard]] constexpr const RoomVisualProfile& room_visual_profile(
    int room_index) noexcept {
    const int normalized = (room_index % 6 + 6) % 6;
    return kRoomVisualProfiles[static_cast<std::size_t>(normalized)];
}

}  // namespace deep_shelter::render
