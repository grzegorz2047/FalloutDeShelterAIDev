#include "assets/GeneratedDwellerAtlas.hpp"

#include <array>

namespace deep_shelter::assets {
namespace {

constexpr std::uint16_t rgba5551(std::uint8_t r,
                                 std::uint8_t g,
                                 std::uint8_t b,
                                 bool opaque = true) noexcept {
    return static_cast<std::uint16_t>(
        ((static_cast<std::uint16_t>(r) >> 3u) << 11u) |
        ((static_cast<std::uint16_t>(g) >> 3u) << 6u) |
        ((static_cast<std::uint16_t>(b) >> 3u) << 1u) |
        (opaque ? 1u : 0u));
}

constexpr std::size_t pica_tile_offset(std::size_t x, std::size_t y) noexcept {
    const std::size_t tile_x = x / 8u;
    const std::size_t tile_y = y / 8u;
    const std::size_t local_x = x & 7u;
    const std::size_t local_y = y & 7u;
    const std::size_t tile_index =
        tile_y * (kGeneratedDwellerAtlasWidth / 8u) + tile_x;
    const std::size_t morton =
        (local_x & 1u) |
        ((local_y & 1u) << 1u) |
        ((local_x & 2u) << 1u) |
        ((local_y & 2u) << 2u) |
        ((local_x & 4u) << 2u) |
        ((local_y & 4u) << 3u);
    return tile_index * 64u + morton;
}

struct DwellerPalette {
    std::uint16_t outline;
    std::uint16_t skin;
    std::uint16_t hair;
    std::uint16_t shirt;
    std::uint16_t accent;
    std::uint16_t trousers;
    std::uint16_t boots;
};

constexpr std::array<DwellerPalette, kGeneratedDwellerArchetypeCount> kPalettes{{
    {rgba5551(24, 28, 30), rgba5551(226, 174, 126), rgba5551(77, 50, 30),
     rgba5551(39, 93, 132), rgba5551(237, 177, 48), rgba5551(34, 54, 69),
     rgba5551(30, 27, 25)},
    {rgba5551(24, 28, 25), rgba5551(210, 158, 112), rgba5551(92, 53, 30),
     rgba5551(55, 112, 65), rgba5551(130, 190, 82), rgba5551(75, 68, 45),
     rgba5551(31, 29, 24)},
    {rgba5551(21, 29, 33), rgba5551(223, 170, 126), rgba5551(58, 42, 30),
     rgba5551(36, 102, 126), rgba5551(75, 185, 211), rgba5551(38, 61, 73),
     rgba5551(24, 28, 31)},
    {rgba5551(31, 25, 23), rgba5551(219, 163, 112), rgba5551(49, 34, 27),
     rgba5551(128, 65, 42), rgba5551(219, 125, 53), rgba5551(63, 54, 48),
     rgba5551(28, 25, 24)},
    {rgba5551(28, 25, 25), rgba5551(230, 179, 135), rgba5551(78, 48, 31),
     rgba5551(108, 49, 48), rgba5551(184, 105, 70), rgba5551(45, 58, 72),
     rgba5551(27, 26, 26)},
}};

constexpr bool inside(int x,
                      int y,
                      int left,
                      int top,
                      int right,
                      int bottom) noexcept {
    return x >= left && x <= right && y >= top && y <= bottom;
}

std::uint16_t sprite_pixel(DwellerArchetype archetype,
                           DwellerAnimation animation,
                           std::size_t frame,
                           int x,
                           int y) noexcept {
    const auto palette = kPalettes[static_cast<std::size_t>(archetype)];
    const int idle_bob[4] = {0, -1, 0, 1};
    const int bob = animation == DwellerAnimation::Idle ? idle_bob[frame] : 0;
    const int px = x;
    const int py = y - bob;
    const bool walk_a = (frame & 1u) == 0u;

    if (inside(px, py, 7, 29, 17, 30)) return rgba5551(12, 14, 15);

    int left_leg_x = 7;
    int right_leg_x = 13;
    if (animation == DwellerAnimation::Walk) {
        left_leg_x += walk_a ? -2 : 2;
        right_leg_x += walk_a ? 2 : -2;
    }
    if (inside(px, py, left_leg_x, 22, left_leg_x + 3, 29) ||
        inside(px, py, right_leg_x, 22, right_leg_x + 3, 29)) {
        return py >= 28 ? palette.boots : palette.trousers;
    }

    if (inside(px, py, 6, 12, 17, 23)) {
        if (px == 6 || px == 17 || py == 12 || py == 23) return palette.outline;
        if ((archetype == DwellerArchetype::Technician ||
             archetype == DwellerArchetype::WaterOperator) &&
            px >= 13 && py <= 18) {
            return palette.accent;
        }
        if (archetype == DwellerArchetype::Civilian && px == 11) {
            return palette.accent;
        }
        return palette.shirt;
    }

    int left_arm_top = 14;
    int right_arm_top = 14;
    int left_arm_x = 3;
    int right_arm_x = 18;
    if (animation == DwellerAnimation::Work) {
        left_arm_top = (frame == 1u || frame == 2u) ? 12 : 16;
        right_arm_top = (frame == 0u || frame == 3u) ? 12 : 16;
        left_arm_x = frame == 2u ? 2 : 4;
        right_arm_x = frame == 1u ? 19 : 17;
    } else if (animation == DwellerAnimation::Walk) {
        left_arm_top += walk_a ? -2 : 2;
        right_arm_top += walk_a ? 2 : -2;
    }
    if (inside(px, py, left_arm_x, left_arm_top, left_arm_x + 2, left_arm_top + 8) ||
        inside(px, py, right_arm_x, right_arm_top, right_arm_x + 2, right_arm_top + 8)) {
        return py >= left_arm_top + 6 || py >= right_arm_top + 6
                   ? palette.skin
                   : palette.shirt;
    }

    if (animation == DwellerAnimation::Work) {
        const int tool_x = frame < 2u ? 1 : 20;
        if (inside(px, py, tool_x, 19, tool_x + 2, 25)) return palette.accent;
        if (inside(px, py, tool_x - 1, 18, tool_x + 3, 19)) return palette.outline;
    }

    if (inside(px, py, 7, 4, 16, 13)) {
        if (px == 7 || px == 16 || py == 4 || py == 13) return palette.outline;
        if (py <= 7 && (archetype == DwellerArchetype::Technician ||
                        archetype == DwellerArchetype::Gardener ||
                        archetype == DwellerArchetype::WaterOperator)) {
            return palette.accent;
        }
        if ((py == 6 || px == 8 || px == 15) &&
            archetype != DwellerArchetype::WaterOperator) {
            return palette.hair;
        }
        if (py == 10 && (px == 9 || px == 14)) return palette.outline;
        return palette.skin;
    }

    if (archetype == DwellerArchetype::Technician &&
        inside(px, py, 5, 3, 18, 5)) {
        return py == 3 ? palette.outline : palette.accent;
    }
    if (archetype == DwellerArchetype::Mechanic &&
        inside(px, py, 6, 4, 17, 5)) {
        return palette.accent;
    }

    return 0u;
}

std::uint16_t decoded_pixel(std::size_t pixel_index) noexcept {
    const std::size_t x = pixel_index % kGeneratedDwellerAtlasWidth;
    const std::size_t y = pixel_index / kGeneratedDwellerAtlasWidth;
    const std::size_t column = x / kGeneratedDwellerFrameWidth;
    const std::size_t row = y / kGeneratedDwellerFrameHeight;
    if (column >= kGeneratedDwellerAtlasColumns ||
        row >= kGeneratedDwellerAtlasRows) {
        return 0u;
    }

    const std::size_t linear_frame = row * kGeneratedDwellerAtlasColumns + column;
    if (linear_frame >= kGeneratedDwellerFrameCount) return 0u;

    const std::size_t frames_per_archetype =
        kGeneratedDwellerAnimationCount * kGeneratedDwellerFramesPerAnimation;
    const auto archetype = static_cast<DwellerArchetype>(
        linear_frame / frames_per_archetype);
    const std::size_t local_frame = linear_frame % frames_per_archetype;
    const auto animation = static_cast<DwellerAnimation>(
        local_frame / kGeneratedDwellerFramesPerAnimation);
    const std::size_t frame =
        local_frame % kGeneratedDwellerFramesPerAnimation;
    return sprite_pixel(archetype,
                        animation,
                        frame,
                        static_cast<int>(x % kGeneratedDwellerFrameWidth),
                        static_cast<int>(y % kGeneratedDwellerFrameHeight));
}

}  // namespace

DwellerArchetype dweller_archetype_for_room(int room_index) noexcept {
    switch ((room_index % 6 + 6) % 6) {
        case 0: return DwellerArchetype::Technician;
        case 1: return DwellerArchetype::Gardener;
        case 2: return DwellerArchetype::WaterOperator;
        case 3: return DwellerArchetype::Mechanic;
        default: return DwellerArchetype::Civilian;
    }
}

DwellerArchetype safe_dweller_archetype(int raw_archetype) noexcept {
    if (raw_archetype < 0 ||
        raw_archetype >= static_cast<int>(kGeneratedDwellerArchetypeCount)) {
        return DwellerArchetype::Civilian;
    }
    return static_cast<DwellerArchetype>(raw_archetype);
}

DwellerAtlasRegion dweller_atlas_region(DwellerArchetype archetype,
                                        DwellerAnimation animation,
                                        std::size_t frame) noexcept {
    const std::size_t archetype_index =
        static_cast<std::size_t>(safe_dweller_archetype(
            static_cast<int>(archetype)));
    const std::size_t animation_index =
        static_cast<std::size_t>(animation) < kGeneratedDwellerAnimationCount
            ? static_cast<std::size_t>(animation)
            : static_cast<std::size_t>(DwellerAnimation::Idle);
    const std::size_t safe_frame = frame % kGeneratedDwellerFramesPerAnimation;
    const std::size_t linear_frame =
        (archetype_index * kGeneratedDwellerAnimationCount + animation_index) *
            kGeneratedDwellerFramesPerAnimation +
        safe_frame;
    return {
        static_cast<std::uint16_t>(
            (linear_frame % kGeneratedDwellerAtlasColumns) *
            kGeneratedDwellerFrameWidth),
        static_cast<std::uint16_t>(
            (linear_frame / kGeneratedDwellerAtlasColumns) *
            kGeneratedDwellerFrameHeight),
        static_cast<std::uint16_t>(kGeneratedDwellerFrameWidth),
        static_cast<std::uint16_t>(kGeneratedDwellerFrameHeight),
    };
}

std::size_t dweller_animation_frame(std::uint32_t simulation_tick,
                                    DwellerAnimation animation,
                                    std::uint32_t phase) noexcept {
    std::uint32_t ticks_per_frame = 12u;
    if (animation == DwellerAnimation::Work) ticks_per_frame = 8u;
    if (animation == DwellerAnimation::Walk) ticks_per_frame = 5u;
    return static_cast<std::size_t>(
        (simulation_tick / ticks_per_frame + phase) %
        kGeneratedDwellerFramesPerAnimation);
}

void decode_generated_dweller_atlas(std::uint16_t* output,
                                    std::size_t output_pixels) noexcept {
    if (output == nullptr || output_pixels < kGeneratedDwellerPixelCount) return;
    for (std::size_t pixel = 0; pixel < kGeneratedDwellerPixelCount; ++pixel) {
        output[pixel] = decoded_pixel(pixel);
    }
}

void decode_generated_dweller_atlas_tiled(std::uint16_t* output,
                                          std::size_t output_pixels) noexcept {
    if (output == nullptr || output_pixels < kGeneratedDwellerPixelCount) return;
    for (std::size_t y = 0; y < kGeneratedDwellerAtlasHeight; ++y) {
        for (std::size_t x = 0; x < kGeneratedDwellerAtlasWidth; ++x) {
            const std::size_t source = y * kGeneratedDwellerAtlasWidth + x;
            output[pica_tile_offset(x, y)] = decoded_pixel(source);
        }
    }
}

}  // namespace deep_shelter::assets
