#pragma once

#include <cstddef>

namespace deep_shelter::render::layout {

constexpr int kGridColumns = 8;
constexpr int kGridFloors = 6;
constexpr float kRoomWidth = 132.0f;
constexpr float kRoomHeight = 64.0f;
constexpr float kRoomGapX = 28.0f;
constexpr float kRoomGapY = 4.0f;
constexpr float kWorldPaddingX = 28.0f;
constexpr float kWorldPaddingY = 20.0f;
constexpr float kElevatorWidth = 30.0f;
constexpr float kRoomPitchX = kRoomWidth + kRoomGapX;
constexpr float kRoomPitchY = kRoomHeight + kRoomGapY;
constexpr float kWorldWidth =
    kWorldPaddingX * 2.0f +
    static_cast<float>(kGridColumns) * kRoomWidth +
    static_cast<float>(kGridColumns - 1) * kRoomGapX;
constexpr float kWorldHeight =
    kWorldPaddingY * 2.0f +
    static_cast<float>(kGridFloors) * kRoomHeight +
    static_cast<float>(kGridFloors - 1) * kRoomGapY;
constexpr float kBackdropX = 0.0f;
constexpr float kBackdropY = 0.0f;
constexpr float kBackdropWidth = kWorldWidth;
constexpr float kBackdropHeight = kWorldHeight;
constexpr float kFramingScale = 1.8f;

[[nodiscard]] constexpr bool valid_grid_position(int column,
                                                 int floor) noexcept {
    return column >= 0 && column < kGridColumns &&
           floor >= 0 && floor < kGridFloors;
}

[[nodiscard]] constexpr float room_x(int column) noexcept {
    return kWorldPaddingX + static_cast<float>(column) * kRoomPitchX;
}

[[nodiscard]] constexpr float room_y(int floor) noexcept {
    return kWorldPaddingY + static_cast<float>(floor) * kRoomPitchY;
}

[[nodiscard]] constexpr float elevator_x(int column) noexcept {
    return room_x(column) + (kRoomWidth - kElevatorWidth) * 0.5f;
}

constexpr float kEstimatedPixelsPerFramedUnit = 0.535f;
constexpr float kEstimatedRoomPixelWidth =
    kRoomWidth * kFramingScale * kEstimatedPixelsPerFramedUnit;
constexpr float kEstimatedRoomPixelHeight =
    kRoomHeight * kFramingScale * kEstimatedPixelsPerFramedUnit;

// Visual quality now takes priority over the previous 4096-vertex target.
// The richer renderer may use separate structure, prop and glow passes.
constexpr std::size_t kVerticesPerBox = 36;
constexpr std::size_t kVerticesPerBillboard = 6;
constexpr std::size_t kMaxSceneVertices = 8192;
constexpr std::size_t kMaxSceneBoxes = kMaxSceneVertices / kVerticesPerBox;
constexpr std::size_t kMaximumRoomEntries =
    static_cast<std::size_t>(kGridColumns * kGridFloors);
constexpr std::size_t kMaximumResidentEntries = 12;
constexpr std::size_t kMinimumResidentEntries = 3;
// At the minimum 0.5 camera zoom, at most six room columns and all six
// floors intersect the 400x240 viewport. The remainder is culled.
constexpr std::size_t kMaximumCameraVisibleRooms = 36;
constexpr std::size_t kFullDetailVisibleRoomLimit = 18;
constexpr std::size_t kStaticSceneBoxes = 1;
constexpr std::size_t kActiveRoomBaseBoxes = 5;
constexpr std::size_t kElevatorBaseBoxes = 5;
constexpr std::size_t kMaxRoomSignatureBoxes = 14;
constexpr std::size_t kSelectionBoxes = 8;
constexpr std::size_t kResourceFillBoxes = 1;
constexpr std::size_t kBuildPreviewBoxes = 10;
constexpr std::size_t kOverviewBillboardsPerRoom = 1;
constexpr std::size_t kWorstCaseSceneBoxes =
    kStaticSceneBoxes +
    kMaximumCameraVisibleRooms *
        (kActiveRoomBaseBoxes > kElevatorBaseBoxes
             ? kActiveRoomBaseBoxes
             : kElevatorBaseBoxes) +
    kSelectionBoxes + kResourceFillBoxes + kBuildPreviewBoxes;
constexpr std::size_t kWorstCaseSceneBillboards =
    kMaximumCameraVisibleRooms * kOverviewBillboardsPerRoom;
constexpr std::size_t kWorstCaseSceneVertices =
    kWorstCaseSceneBoxes * kVerticesPerBox +
    kWorstCaseSceneBillboards * kVerticesPerBillboard;
constexpr std::size_t kTargetRenderPasses = 3;

[[nodiscard]] constexpr bool overlaps(float ax,
                                      float ay,
                                      float aw,
                                      float ah,
                                      float bx,
                                      float by,
                                      float bw,
                                      float bh) noexcept {
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

}  // namespace deep_shelter::render::layout
