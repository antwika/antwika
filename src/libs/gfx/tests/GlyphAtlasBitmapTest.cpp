#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include <antwika/font/Coverage.hpp>
#include <antwika/font/GlyphAtlas.hpp>

#include "antwika/gfx/Bitmap.hpp"
#include "antwika/gfx/GfxError.hpp"
#include "antwika/gfx/GlyphAtlasBitmap.hpp"

using antwika::font::Coverage;
using antwika::font::GlyphAtlas;
using antwika::gfx::Bitmap;
using antwika::gfx::GfxError;
using antwika::gfx::glyphAtlasBitmap;
using antwika::gfx::Size;

namespace
{
    // A hand-built atlas rather than one makeGlyphAtlas produced.
    // A GlyphAtlas is a plain value, so this needs no font at all.
    // No bytes and no rasteriser appear anywhere in the test.
    [[nodiscard]] GlyphAtlas atlasWithMask(
        std::uint32_t width,
        std::uint32_t height,
        std::vector<std::uint8_t> samples)
    {
        GlyphAtlas atlas;
        atlas.coverage = Coverage{
            .width = width,
            .height = height,
            .samples = std::move(samples)};

        return atlas;
    }

    // A wrapper so a refusal can be asserted on.
    // glyphAtlasBitmap is [[nodiscard]], and EXPECT_THROW discards.
    Bitmap bitmapOf(const GlyphAtlas &atlas)
    {
        return glyphAtlasBitmap(atlas);
    }
} // namespace

TEST(GlyphAtlasBitmapTest, GlyphAtlasBitmap_IsWhiteWithCoverageInAlpha)
{
    const Bitmap bitmap = bitmapOf(atlasWithMask(2, 1, {0, 128}));

    EXPECT_EQ(bitmap.size, (Size{.width = 2, .height = 1}));
    EXPECT_EQ(
        bitmap.pixels,
        (std::vector<std::uint8_t>{255, 255, 255, 0, 255, 255, 255, 128}));
}

TEST(GlyphAtlasBitmapTest, GlyphAtlasBitmap_ProducesAnUploadableBitmap)
{
    EXPECT_TRUE(
        bitmapOf(atlasWithMask(2, 2, {1, 2, 3, 4})).isComplete());
}

TEST(GlyphAtlasBitmapTest, GlyphAtlasBitmap_RefusesAMaskOfNoWidth)
{
    EXPECT_THROW(bitmapOf(atlasWithMask(0, 4, {})), GfxError);
}

TEST(GlyphAtlasBitmapTest, GlyphAtlasBitmap_RefusesAMaskOfNoHeight)
{
    EXPECT_THROW(bitmapOf(atlasWithMask(4, 0, {})), GfxError);
}

TEST(GlyphAtlasBitmapTest, GlyphAtlasBitmap_RefusesAnIncompleteMask)
{
    EXPECT_THROW(bitmapOf(atlasWithMask(4, 4, {1, 2})), GfxError);
}
