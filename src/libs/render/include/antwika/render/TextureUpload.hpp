#pragma once

#include <memory>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/ITexture.hpp>

namespace antwika::render
{

    inline void layBitmapIntoTexture(
        gfx::IRenderer &viewportRenderer,
        std::unique_ptr<gfx::ITexture> &texture,
        const gfx::Bitmap &bitmap)
    {
        if (texture && texture->getSize() == bitmap.size)
        {
            viewportRenderer.updateTexture(*texture, bitmap);

            return;
        }

        texture = viewportRenderer.createTexture(bitmap);
    }

}
