#include "antwika/gfx/GlyphAtlasBitmap.hpp"

#include <cstdint>

#include <antwika/font/GlyphAtlas.hpp>

#include "antwika/gfx/Bitmap.hpp"
#include "antwika/gfx/GfxError.hpp"

namespace antwika::gfx
{

    Bitmap glyphAtlasBitmap(const font::GlyphAtlas &atlas)
    {
        if (atlas.coverage.width == 0 || atlas.coverage.height == 0)
        {
            throw GfxError(
                "gfx: an atlas with no mask in it is not a texture");
        }

        if (!atlas.coverage.isComplete())
        {
            throw GfxError(
                "gfx: an atlas mask holding fewer samples than its size "
                "claims cannot be expanded");
        }

        Bitmap bitmap{
            .size = {atlas.coverage.width, atlas.coverage.height},
            .pixels = {}};
        bitmap.pixels.reserve(
            atlas.coverage.samples.size() * kBytesPerPixel);

        for (const std::uint8_t sample : atlas.coverage.samples)
        {
            bitmap.pixels.insert(
                bitmap.pixels.end(), {255, 255, 255, sample});
        }

        return bitmap;
    } // GCOVR_EXCL_LINE

} // namespace antwika::gfx
