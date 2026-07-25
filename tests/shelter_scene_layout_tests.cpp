#include <cassert>

#include "render/ShelterSceneLayout.hpp"

using namespace deep_shelter::render::layout;

int main() {
    static_assert(kEstimatedRoomPixelWidth >= 120.0f);
    static_assert(kEstimatedRoomPixelWidth <= 180.0f);
    static_assert(kEstimatedRoomPixelHeight >= 55.0f);
    static_assert(kEstimatedRoomPixelHeight <= 80.0f);
    static_assert(kElevatorWidth < kRoomWidth * 0.30f);
    static_assert(kBackdropWidth / kWorldWidth >= 0.95f);
    static_assert(kBackdropHeight / kWorldHeight >= 0.90f);

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

        for (std::size_t other = room + 1; other < kRoomX.size(); ++other) {
            assert(!overlaps(kRoomX[room],
                             kRoomY[room],
                             kRoomWidth,
                             kRoomHeight,
                             kRoomX[other],
                             kRoomY[other],
                             kRoomWidth,
                             kRoomHeight));
        }
    }

    for (std::size_t floor = 1; floor < 3; ++floor) {
        const float previous_bottom = kRoomY[(floor - 1) * 2] + kRoomHeight;
        const float current_top = kRoomY[floor * 2];
        assert(current_top - previous_bottom >= kRoomGapY);
    }

    return 0;
}
