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
    static_assert(kElevatorWidth < kRoomWidth * 0.30f);
    static_assert(kBackdropWidth / kWorldWidth >= 0.95f);
    static_assert(kBackdropHeight / kWorldHeight >= 0.90f);
    static_assert(kUnbuiltRoomBoxes <
                  kActiveRoomBaseBoxes + kMaxRoomSignatureBoxes);
    static_assert(kWorstCaseSceneBoxes <= kMaxSceneBoxes);
    static_assert(kWorstCaseSceneVertices <= kMaxSceneVertices);
    static_assert(kTargetRenderPasses >= 3);
    static_assert(kRoomVisualProfiles.size() == kRoomX.size());

    for (std::size_t room = 0; room < kRoomX.size(); ++room) {
        assert(kRoomX[room] >= 0.0f);
        assert(kRoomY[room] >= 0.0f);
        assert(kRoomX[room] + kRoomWidth <= kWorldWidth);
        assert(kRoomY[room] + kRoomHeight <= kWorldHeight);
        assert(!overlaps(kRoomX[room],
                         kRoomY[room],
                         kRoomWidth,
                         kRoomHeight,
                         kElevatorX,
                         kElevatorY,
                         kElevatorWidth,
                         kElevatorHeight));

        const auto& profile = kRoomVisualProfiles[room];
        assert(static_cast<std::size_t>(profile.dominant_prop) == room);
        assert(profile.secondary_prop_count >= 3);
        assert(profile.dominant_width >= 50.0f);
        assert(profile.dominant_height >= 28.0f);
        assert(profile.resident_clearance_width >= 22.0f);
        assert(profile.dominant_width + profile.resident_clearance_width <=
               kRoomWidth - 4.0f);

        for (std::size_t other = room + 1; other < kRoomX.size(); ++other) {
            assert(!overlaps(kRoomX[room],
                             kRoomY[room],
                             kRoomWidth,
                             kRoomHeight,
                             kRoomX[other],
                             kRoomY[other],
                             kRoomWidth,
                             kRoomHeight));
            assert(kRoomVisualProfiles[room].dominant_prop !=
                   kRoomVisualProfiles[other].dominant_prop);
        }
    }

    for (std::size_t floor = 1; floor < 3; ++floor) {
        const float previous_bottom = kRoomY[(floor - 1) * 2] + kRoomHeight;
        const float current_top = kRoomY[floor * 2];
        assert(current_top - previous_bottom >= kRoomGapY);
    }

    return 0;
}
