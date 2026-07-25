#pragma once

#include <array>
#include <cstddef>

namespace deep_shelter::render::layout {

constexpr float kWorldWidth = 400.0f;
constexpr float kWorldHeight = 240.0f;
constexpr float kRoomWidth = 132.0f;
constexpr float kRoomHeight = 64.0f;
constexpr float kRoomGapY = 4.0f;
constexpr float kElevatorX = 185.0f;
constexpr float kElevatorY = 18.0f;
constexpr float kElevatorWidth = 30.0f;
constexpr float kElevatorHeight = 204.0f;
constexpr float kBackdropX = 4.0f;
constexpr float kBackdropY = 10.0f;
constexpr float kBackdropWidth = 392.0f;
constexpr float kBackdropHeight = 220.0f;
constexpr float kFramingScale = 1.8f;

constexpr std::array<float, 6> kRoomX{{18.0f, 250.0f,
                                        18.0f, 250.0f,
                                        18.0f, 250.0f}};
constexpr std::array<float, 6> kRoomY{{22.0f, 22.0f,
                                        90.0f, 90.0f,
                                        158.0f, 158.0f}};

constexpr float kEstimatedPixelsPerFramedUnit = 0.535f;
constexpr float kEstimatedRoomPixelWidth =
    kRoomWidth * kFramingScale * kEstimatedPixelsPerFramedUnit;
constexpr float kEstimatedRoomPixelHeight =
    kRoomHeight * kFramingScale * kEstimatedPixelsPerFramedUnit;

// Visual quality now takes priority over the previous 4096-vertex target.
// The richer renderer may use separate structure, prop and glow passes.
constexpr std::size_t kVerticesPerBox = 36;
constexpr std::size_t kMaxSceneVertices = 8192;
constexpr std::size_t kMaxSceneBoxes = kMaxSceneVertices / kVerticesPerBox;
constexpr std::size_t kStaticSceneBoxes = 22;
constexpr std::size_t kActiveRoomBaseBoxes = 7;
constexpr std::size_t kMaxRoomSignatureBoxes = 14;
constexpr std::size_t kUnbuiltRoomBoxes = 8;
constexpr std::size_t kSelectionBoxes = 8;
constexpr std::size_t kResidentBoxes = 2;
constexpr std::size_t kResourceFillBoxes = 1;
constexpr std::size_t kGlowBoxesPerActiveRoom = 2;
constexpr std::size_t kWorstCaseSceneBoxes =
    kStaticSceneBoxes +
    kRoomX.size() * (kActiveRoomBaseBoxes + kMaxRoomSignatureBoxes +
                     kGlowBoxesPerActiveRoom) +
    kSelectionBoxes + kResidentBoxes + kResourceFillBoxes;
constexpr std::size_t kWorstCaseSceneVertices =
    kWorstCaseSceneBoxes * kVerticesPerBox;
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
