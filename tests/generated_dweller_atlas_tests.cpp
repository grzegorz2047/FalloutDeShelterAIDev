#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

#include "assets/GeneratedDwellerAtlas.hpp"

namespace {

using deep_shelter::assets::DwellerAnimation;
using deep_shelter::assets::DwellerArchetype;

constexpr std::size_t tiled_offset(std::size_t x, std::size_t y) noexcept {
    using namespace deep_shelter::assets;
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

}  // namespace

int main() {
    using namespace deep_shelter::assets;

    static_assert(kGeneratedDwellerFrameCount == 60);
    static_assert(kGeneratedDwellerRuntimeBytes == 128u * 1024u);
    static_assert(kGeneratedDwellerAtlasColumns * kGeneratedDwellerFrameWidth <=
                  kGeneratedDwellerAtlasWidth);
    static_assert(kGeneratedDwellerAtlasRows * kGeneratedDwellerFrameHeight <=
                  kGeneratedDwellerAtlasHeight);

    const auto fallback = safe_dweller_archetype(999);
    assert(fallback == DwellerArchetype::Civilian);
    assert(dweller_archetype_for_room(0) == DwellerArchetype::Technician);
    assert(dweller_archetype_for_room(1) == DwellerArchetype::Gardener);
    assert(dweller_archetype_for_room(2) == DwellerArchetype::WaterOperator);
    assert(dweller_archetype_for_room(3) == DwellerArchetype::Mechanic);
    assert(dweller_archetype_for_room(4) == DwellerArchetype::Civilian);
    assert(dweller_archetype_for_room(5) == DwellerArchetype::Civilian);

    std::array<std::uint16_t, kGeneratedDwellerPixelCount> linear{};
    std::array<std::uint16_t, kGeneratedDwellerPixelCount> tiled{};
    decode_generated_dweller_atlas(linear.data(), linear.size());
    decode_generated_dweller_atlas_tiled(tiled.data(), tiled.size());

    for (std::size_t y = 0; y < kGeneratedDwellerAtlasHeight; ++y) {
        for (std::size_t x = 0; x < kGeneratedDwellerAtlasWidth; ++x) {
            assert(linear[y * kGeneratedDwellerAtlasWidth + x] ==
                   tiled[tiled_offset(x, y)]);
        }
    }

    std::array<std::uint16_t, kGeneratedDwellerArchetypeCount> signatures{};
    for (std::size_t archetype = 0;
         archetype < kGeneratedDwellerArchetypeCount;
         ++archetype) {
        std::size_t opaque_pixels = 0;
        for (std::size_t animation = 0;
             animation < kGeneratedDwellerAnimationCount;
             ++animation) {
            for (std::size_t frame = 0;
                 frame < kGeneratedDwellerFramesPerAnimation;
                 ++frame) {
                const auto region = dweller_atlas_region(
                    static_cast<DwellerArchetype>(archetype),
                    static_cast<DwellerAnimation>(animation),
                    frame);
                assert(region.x + region.width <= kGeneratedDwellerAtlasWidth);
                assert(region.y + region.height <= kGeneratedDwellerAtlasHeight);
                for (std::size_t y = region.y; y < region.y + region.height; ++y) {
                    for (std::size_t x = region.x; x < region.x + region.width; ++x) {
                        if ((linear[y * kGeneratedDwellerAtlasWidth + x] & 1u) != 0u) {
                            ++opaque_pixels;
                        }
                    }
                }
                if (animation == 0u && frame == 0u) {
                    signatures[archetype] =
                        linear[(region.y + 6u) * kGeneratedDwellerAtlasWidth +
                               region.x + 12u];
                }
            }
        }
        assert(opaque_pixels > 1400u);
    }

    for (std::size_t i = 0; i < signatures.size(); ++i) {
        for (std::size_t j = i + 1; j < signatures.size(); ++j) {
            assert(signatures[i] != signatures[j]);
        }
    }

    assert(dweller_animation_frame(0, DwellerAnimation::Idle, 0) == 0);
    assert(dweller_animation_frame(12, DwellerAnimation::Idle, 0) == 1);
    assert(dweller_animation_frame(8, DwellerAnimation::Work, 0) == 1);
    assert(dweller_animation_frame(5, DwellerAnimation::Walk, 0) == 1);
    assert(dweller_animation_frame(0, DwellerAnimation::Idle, 3) == 3);

    const auto final_region = dweller_atlas_region(
        DwellerArchetype::Civilian, DwellerAnimation::Walk, 3);
    assert(final_region.x == 216);
    assert(final_region.y == 160);
    return 0;
}
