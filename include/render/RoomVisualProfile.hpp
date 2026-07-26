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
    float resident_clear_x;
    bool uses_translucent_glow;
};

constexpr std::array<RoomVisualProfile, 6> kRoomVisualProfiles{{
    {DominantProp::Generator, 5, 58.0f, 42.0f, 31.0f, 101.0f, true},
    {DominantProp::Planter, 5, 104.0f, 28.0f, 24.0f, 91.0f, true},
    {DominantProp::WaterTank, 5, 72.0f, 43.0f, 26.0f, 12.0f, true},
    {DominantProp::Workbench, 5, 92.0f, 31.0f, 28.0f, 62.0f, true},
    {DominantProp::StorageRack, 5, 83.0f, 45.0f, 22.0f, 8.0f, false},
    {DominantProp::BunkBed, 5, 76.0f, 36.0f, 30.0f, 93.0f, true},
}};

[[nodiscard]] constexpr const RoomVisualProfile& room_visual_profile(
    int room_index) noexcept {
    const int normalized = (room_index % 6 + 6) % 6;
    return kRoomVisualProfiles[static_cast<std::size_t>(normalized)];
}

}  // namespace deep_shelter::render
