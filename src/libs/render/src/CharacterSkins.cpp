#include "antwika/render/CharacterSkins.hpp"

#include <cstddef>
#include <utility>

#include "antwika/render/TextureUpload.hpp"

namespace antwika::render
{

    void CharacterSkins::take(
        gfx::IRenderer &viewportRenderer,
        std::vector<gfx::Bitmap> takenBitmaps)
    {
        skins = std::move(takenBitmaps);
        pictureTextures.resize(skins.size());

        for (std::size_t skinIndex = 0; skinIndex < skins.size();
             ++skinIndex)
        {
            layBitmapIntoTexture(
                viewportRenderer,
                pictureTextures.at(skinIndex),
                skins.at(skinIndex));
        }
    }

    const std::vector<gfx::Bitmap> &CharacterSkins::getSheets()
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
        layBitmapIntoTexture(
            viewportRenderer,
            pictureTextures.at(skinIndex),
            skins.at(skinIndex));
    }

    gfx::ITexture *CharacterSkins::getPicture(
        const std::size_t skinIndex) const noexcept
    {
        return skinIndex < pictureTextures.size()
                   ? pictureTextures.at(skinIndex).get()
                   : nullptr;
    }

    std::size_t CharacterSkins::getSize() const noexcept
    {
        return skins.size();
    }

}
