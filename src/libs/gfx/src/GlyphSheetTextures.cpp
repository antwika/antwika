#include "antwika/gfx/GlyphSheetTextures.hpp"

#include <cstdint>
#include <string_view>
#include <utility>

#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/GlyphBlit.hpp"
#include "antwika/gfx/GlyphCells.hpp"
#include "antwika/gfx/GlyphSheet.hpp"
#include "antwika/gfx/IRenderer.hpp"
#include "antwika/gfx/Point.hpp"

namespace antwika::gfx
{

    void GlyphSheetTextures::draw(
        IRenderer &renderer,
        Point origin,
        std::string_view text,
        std::uint32_t scale,
        Color color)
    {
        if (scale == 0 || text.empty())
        {
            return;
        }

        const GlyphCells &baked = cells.at(scale);

        auto sheet = sheets.find(scale);

        if (sheet == sheets.end())
        {
            sheet = sheets
                        .emplace(
                            scale,
                            renderer.createTexture(
                                glyphSheetBitmap(baked)))
                        .first;
        }

        if (sheet->second == nullptr)
        {
            return;
        }

        for (const GlyphBlit &blit :
             glyphSheetBlits(baked, origin, text))
        {
            renderer.drawTexture(
                *sheet->second, blit.source, blit.destination, color);
        }
    }

}
