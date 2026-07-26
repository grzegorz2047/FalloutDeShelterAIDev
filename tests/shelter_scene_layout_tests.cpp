#include <cassert>

#include "render/RoomVisualProfile.hpp"
#include "render/ShelterSceneLayout.hpp"

using namespace deep_shelter::render;
using namespace deep_shelter::render::layout;

int main() {
    static_assert(kEstimatedRoomPixelWidth >= 120.0f);
    static_assert(kEstimatedRoomPixelWidth <= 180.0f);
    static_assert(kEstimatedRoomPixelHeight >= 55.0f);
    static_assert(kEstimatedRoomPixelHeight <= 80.0f);
    static_assert(kGridColumns >= 8);
    static_assert(kGridFloors >= 6);
    static_assert(kWorldWidth > 400.0f);
    static_assert(kWorldHeight > 240.0f);
    static_assert(kElevatorWidth < kRoomWidth * 0.30f);
    static_assert(kBackdropWidth == kWorldWidth);
    static_assert(kBackdropHeight == kWorldHeight);
    static_assert(kMaximumRoomEntries >= 48);
    static_assert(kMaximumResidentEntries >= 3);
    static_assert(kMaximumCameraVisibleRooms <
                  kMaximumRoomEntries);
    static_assert(kFullDetailVisibleRoomLimit <
                  kMaximumCameraVisibleRooms);
    static_assert(kBuildPreviewBoxes >= 6);
    static_assert(kWorstCaseSceneBoxes <= kMaxSceneBoxes);
    static_assert(kWorstCaseSceneVertices <= kMaxSceneVertices);
    static_assert(kTargetRenderPasses >= 3);

    for (std::size_t profile_index = 0;
         profile_index < kRoomVisualProfiles.size();
         ++profile_index) {
        const auto& profile = kRoomVisualProfiles[profile_index];
        assert(static_cast<std::size_t>(profile.dominant_prop) ==
               profile_index);
        assert(profile.secondary_prop_count >= 3);
        assert(profile.dominant_width >= 50.0f);
        assert(profile.dominant_height >= 28.0f);
        assert(profile.resident_clearance_width >= 22.0f);
        assert(profile.dominant_width + profile.resident_clearance_width <=
               kRoomWidth - 4.0f);

        for (std::size_t other = profile_index + 1;
             other < kRoomVisualProfiles.size();
             ++other) {
            assert(kRoomVisualProfiles[profile_index].dominant_prop !=
                   kRoomVisualProfiles[other].dominant_prop);
        }
    }

    for (int floor = 0; floor < kGridFloors; ++floor) {
        for (int column = 0; column < kGridColumns; ++column) {
            assert(valid_grid_position(column, floor));
            const float x = room_x(column);
            const float y = room_y(floor);
            assert(x >= kWorldPaddingX);
            assert(y >= kWorldPaddingY);
            assert(x + kRoomWidth <= kWorldWidth);
            assert(y + kRoomHeight <= kWorldHeight);
            assert(elevator_x(column) >= x);
            assert(elevator_x(column) + kElevatorWidth <=
                   x + kRoomWidth);

            if (column + 1 < kGridColumns) {
                assert(!overlaps(x,
                                 y,
                                 kRoomWidth,
                                 kRoomHeight,
                                 room_x(column + 1),
                                 y,
                                 kRoomWidth,
                                 kRoomHeight));
                assert(room_x(column + 1) - (x + kRoomWidth) >=
                       kRoomGapX);
            }
            if (floor + 1 < kGridFloors) {
                assert(!overlaps(x,
                                 y,
                                 kRoomWidth,
                                 kRoomHeight,
                                 x,
                                 room_y(floor + 1),
                                 kRoomWidth,
                                 kRoomHeight));
            }
        }
    }

    assert(!valid_grid_position(-1, 0));
    assert(!valid_grid_position(0, -1));
    assert(!valid_grid_position(kGridColumns, 0));
    assert(!valid_grid_position(0, kGridFloors));

    for (int floor = 1; floor < kGridFloors; ++floor) {
        const float previous_bottom =
            room_y(floor - 1) + kRoomHeight;
        const float current_top = room_y(floor);
        assert(current_top - previous_bottom >= kRoomGapY);
    }

    return 0;
}
