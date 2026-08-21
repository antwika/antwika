#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>

namespace antwika::render
{

    class CharacterSkins final
    {
    public:
        void take(
            gfx::ViewportRenderer &viewportRenderer,
            std::vector<gfx::Bitmap> skinBitmaps);

        [[nodiscard]] const std::vector<gfx::Bitmap> &sheets()
            const noexcept;

        void lay(
            gfx::ViewportRenderer &viewportRenderer,
            std::size_t skinIndex,
            gfx::Bitmap skinBitmap);

        [[nodiscard]] gfx::ITexture *picture(
            std::size_t skinIndex) const noexcept;

        [[nodiscard]] std::size_t size() const noexcept;

    private:
        std::vector<gfx::Bitmap> skins;
        std::vector<std::unique_ptr<gfx::ITexture>> pictureTextures;
    };

}
