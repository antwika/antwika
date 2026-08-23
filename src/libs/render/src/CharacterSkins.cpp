#include "antwika/render/CharacterSkins.hpp"

#include <utility>

namespace antwika::render
{

    void CharacterSkins::take(
        gfx::IRenderer &viewportRenderer,
        std::vector<gfx::Bitmap> takenBitmaps)
    {
        skins = std::move(takenBitmaps);
        pictureTextures.clear();

        for (const auto &skin : skins)
        {
            pictureTextures.push_back(viewportRenderer.createTexture(skin));
        }
    }

    const std::vector<gfx::Bitmap> &CharacterSkins::sheets()
        const noexcept
    {
        return skins;
    }

    void CharacterSkins::lay(
        gfx::IRenderer &viewportRenderer,
        const std::size_t skinIndex,
        gfx::Bitmap skinBitmap)
    {
        if (skinIndex >= skins.size())
        {
            return;
        }

        skins.at(skinIndex) = std::move(skinBitmap);
        pictureTextures.at(skinIndex) =
            viewportRenderer.createTexture(skins.at(skinIndex));
    }

    gfx::ITexture *CharacterSkins::picture(
        const std::size_t skinIndex) const noexcept
    {
        return skinIndex < pictureTextures.size()
                   ? pictureTextures.at(skinIndex).get()
                   : nullptr;
    }

    std::size_t CharacterSkins::size() const noexcept
    {
        return skins.size();
    }

}
