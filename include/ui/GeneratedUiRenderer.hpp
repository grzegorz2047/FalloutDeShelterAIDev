#pragma once

#include <citro2d.h>

#include <array>

#include "assets/GeneratedUiAtlas.hpp"

namespace deep_shelter::ui {

class GeneratedUiRenderer {
public:
    GeneratedUiRenderer() = default;
    ~GeneratedUiRenderer();

    GeneratedUiRenderer(const GeneratedUiRenderer&) = delete;
    GeneratedUiRenderer& operator=(const GeneratedUiRenderer&) = delete;

    [[nodiscard]] bool initialize() noexcept;
    void shutdown() noexcept;

    void draw_icon(assets::UiIcon icon,
                   float x,
                   float y,
                   float width,
                   float height,
                   float depth = 0.3f,
                   const C2D_ImageTint* tint = nullptr) const noexcept;

    void draw_button_frame(assets::UiButtonState state,
                           float x,
                           float y,
                           float width,
                           float height,
                           float depth = 0.1f) const noexcept;

    [[nodiscard]] bool initialized() const noexcept { return initialized_; }

private:
    static Tex3DS_SubTexture make_subtexture(assets::UiAtlasRegion region) noexcept;

    C3D_Tex texture_{};
    std::array<Tex3DS_SubTexture,
               static_cast<std::size_t>(assets::UiIcon::Count)>
        icon_regions_{};
    std::array<Tex3DS_SubTexture,
               static_cast<std::size_t>(assets::UiButtonState::Count)>
        button_regions_{};
    bool initialized_ = false;
};

}  // namespace deep_shelter::ui
