#include "ui/GeneratedUiRenderer.hpp"

#include <cstddef>
#include <cstdint>

namespace deep_shelter::ui {

GeneratedUiRenderer::~GeneratedUiRenderer() {
    shutdown();
}

Tex3DS_SubTexture GeneratedUiRenderer::make_subtexture(
    assets::UiAtlasRegion region) noexcept {
    const float atlas_width = static_cast<float>(assets::kGeneratedUiAtlasWidth);
    const float atlas_height = static_cast<float>(assets::kGeneratedUiAtlasHeight);
    const float left = static_cast<float>(region.x) / atlas_width;
    const float right = static_cast<float>(region.x + region.width) / atlas_width;
    const float top = 1.0f - static_cast<float>(region.y) / atlas_height;
    const float bottom =
        1.0f - static_cast<float>(region.y + region.height) / atlas_height;
    return {region.width, region.height, left, top, right, bottom};
}

bool GeneratedUiRenderer::initialize() noexcept {
    if (initialized_) return true;
    shutdown();

    if (!C3D_TexInit(&texture_,
                     static_cast<u16>(assets::kGeneratedUiAtlasWidth),
                     static_cast<u16>(assets::kGeneratedUiAtlasHeight),
                     GPU_RGBA8)) {
        return false;
    }

    assets::decode_generated_ui_atlas_tiled(
        static_cast<std::uint32_t*>(texture_.data),
        assets::kGeneratedUiAtlasPixelCount);
    C3D_TexFlush(&texture_);
    C3D_TexSetFilter(&texture_, GPU_NEAREST, GPU_NEAREST);
    C3D_TexSetWrap(&texture_, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);

    for (std::size_t index = 0; index < icon_regions_.size(); ++index) {
        icon_regions_[index] =
            make_subtexture(assets::ui_icon_region(static_cast<assets::UiIcon>(index)));
    }
    for (std::size_t index = 0; index < button_regions_.size(); ++index) {
        button_regions_[index] = make_subtexture(
            assets::ui_button_region(static_cast<assets::UiButtonState>(index)));
    }

    initialized_ = true;
    return true;
}

void GeneratedUiRenderer::shutdown() noexcept {
    if (!initialized_ && texture_.data == nullptr) return;
    if (texture_.data != nullptr) C3D_TexDelete(&texture_);
    texture_ = {};
    initialized_ = false;
}

void GeneratedUiRenderer::draw_icon(assets::UiIcon icon,
                                    float x,
                                    float y,
                                    float width,
                                    float height,
                                    float depth,
                                    const C2D_ImageTint* tint) const noexcept {
    if (!initialized_) return;
    const std::size_t index = static_cast<std::size_t>(icon);
    if (index >= icon_regions_.size()) return;
    const C2D_Image image{const_cast<C3D_Tex*>(&texture_), &icon_regions_[index]};
    C2D_DrawImageAt(image,
                    x,
                    y,
                    depth,
                    tint,
                    width / static_cast<float>(assets::kGeneratedUiIconSize),
                    height / static_cast<float>(assets::kGeneratedUiIconSize));
}

void GeneratedUiRenderer::draw_button_frame(assets::UiButtonState state,
                                            float x,
                                            float y,
                                            float width,
                                            float height,
                                            float depth) const noexcept {
    if (!initialized_) return;
    const std::size_t index = static_cast<std::size_t>(state);
    if (index >= button_regions_.size()) return;
    const C2D_Image image{const_cast<C3D_Tex*>(&texture_), &button_regions_[index]};
    C2D_DrawImageAt(image,
                    x,
                    y,
                    depth,
                    nullptr,
                    width / static_cast<float>(assets::kGeneratedUiButtonWidth),
                    height / static_cast<float>(assets::kGeneratedUiButtonHeight));
}

}  // namespace deep_shelter::ui
