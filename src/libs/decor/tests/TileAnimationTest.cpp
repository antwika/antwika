#include <gtest/gtest.h>

#include <cstddef>
#include <vector>

#include <antwika/tilemap/AtlasLayout.hpp>
#include <antwika/voxelmap/Voxel.hpp>

#include <antwika/decor/Decor.hpp>
#include <antwika/decor/TileAnimation.hpp>

namespace
{

    using antwika::tilemap::Atlas;
    using antwika::decor::animationFrameAt;
    using antwika::decor::animationOf;
    using antwika::decor::atlasWithAnimationFrames;
    using antwika::decor::anyTileAnimated;
    using antwika::decor::kDecorPaceTick;
    using antwika::decor::kMaxDecorFrames;
    using antwika::tilemap::Tile;
    using antwika::decor::TileAnimation;
    using antwika::tilemap::tileSource;
    using antwika::decor::withAnimationFrameAdded;
    using antwika::decor::withAnimationFrameSet;
    using antwika::decor::withAnimationToggled;

    constexpr Tile kOneTile{.atlas = Atlas::Floor, .index = 1};

    constexpr Tile kOtherTile{.atlas = Atlas::Floor, .index = 2};

    [[nodiscard]] antwika::gfx::Bitmap sheetOf(
        const Atlas atlas, const std::uint8_t fill)
    {
        const auto size = antwika::tilemap::atlasSize(
            antwika::tilemap::tileSizeOf(atlas));

        return antwika::gfx::Bitmap{
            .size = size,
            .pixels = std::vector<std::uint8_t>(
                static_cast<std::size_t>(size.width)
                    * size.height
                    * antwika::gfx::kBytesPerPixel,
                fill)};
    }

    void inkTile(
        antwika::gfx::Bitmap &sheetBitmap,
        const Tile tile,
        const std::uint8_t fill)
    {
        const auto place = tileSource(tile);
        const auto pitch =
            static_cast<std::size_t>(sheetBitmap.size.width)
            * antwika::gfx::kBytesPerPixel;

        for (std::size_t row = 0;
             row < static_cast<std::size_t>(
                 place.size.height);
             ++row)
        {
            const auto byteIndex =
                ((static_cast<std::size_t>(place.originPoint.y)
                  + row)
                 * pitch)
                + (static_cast<std::size_t>(place.originPoint.x)
                   * antwika::gfx::kBytesPerPixel);

            for (std::size_t index = 0;
                 index < static_cast<std::size_t>(
                          place.size.width)
                          * antwika::gfx::kBytesPerPixel;
                 ++index)
            {
                sheetBitmap.pixels.at(byteIndex + index) = fill;
            }
        }
    }

    TEST(TileAnimationTest, WithAnimationToggled_MarksATileAndUnmarksIt)
    {
        const auto toggledAnimations =
            withAnimationToggled(std::vector<TileAnimation>{}, kOneTile);

        ASSERT_NE(animationOf(toggledAnimations, kOneTile), nullptr);
        EXPECT_EQ(animationOf(toggledAnimations, kOneTile)->frameTiles.size(),
            1U);

        const auto untoggledAnimations =
            withAnimationToggled(toggledAnimations, kOneTile);

        EXPECT_EQ(animationOf(untoggledAnimations, kOneTile), nullptr);
    }

    TEST(TileAnimationTest, WithAnimationFrameAdded_HoldsTheCeiling)
    {
        auto flips = withAnimationToggled({}, kOneTile);

        for (std::size_t index = 0; index < kMaxDecorFrames + 2;
             ++index)
        {
            flips = withAnimationFrameAdded(flips, kOneTile);
        }

        EXPECT_EQ(
            animationOf(flips, kOneTile)->frameTiles.size(),
            kMaxDecorFrames);
    }

    TEST(TileAnimationTest, WithAnimationFrameSet_KeepsTheFirstAndTheAtlas)
    {
        auto flips = withAnimationToggled({}, kOneTile);

        flips = withAnimationFrameAdded(flips, kOneTile);
        flips = withAnimationFrameSet(flips, kOneTile, 0, kOtherTile);
        flips = withAnimationFrameSet(
            flips,
            kOneTile,
            1,
            Tile{.atlas = Atlas::Wall, .index = 5});

        EXPECT_EQ(animationOf(flips, kOneTile)->frameTiles.at(0), kOneTile);
        EXPECT_EQ(animationOf(flips, kOneTile)->frameTiles.at(1), kOneTile);

        flips = withAnimationFrameSet(flips, kOneTile, 1, kOtherTile);

        EXPECT_EQ(animationOf(flips, kOneTile)->frameTiles.at(1), kOtherTile);
    }

    TEST(TileAnimationTest, WithAnimationFrameSet_RefusesAnAnimatedSource)
    {
        auto flips = withAnimationToggled({}, kOneTile);

        flips = withAnimationToggled(flips, kOtherTile);
        flips = withAnimationFrameAdded(flips, kOneTile);

        EXPECT_EQ(
            withAnimationFrameSet(flips, kOneTile, 1, kOtherTile), flips);
    }

    TEST(TileAnimationTest, AnyTileAnimated_AsksForMoreThanOneFrame)
    {
        auto flips = withAnimationToggled({}, kOneTile);

        EXPECT_FALSE(anyTileAnimated(flips));

        flips = withAnimationFrameAdded(flips, kOneTile);

        EXPECT_TRUE(anyTileAnimated(flips));
    }

    TEST(TileAnimationTest, AnimationFrameAt_WalksInStepWithTheDecor)
    {
        auto flips = withAnimationToggled({}, kOneTile);

        flips = withAnimationFrameAdded(flips, kOneTile);
        flips = withAnimationFrameSet(flips, kOneTile, 1, kOtherTile);

        const auto &animation = *animationOf(flips, kOneTile);

        EXPECT_EQ(animationFrameAt(animation, 0), kOneTile);
        EXPECT_EQ(
            animationFrameAt(animation, kDecorPaceTick), kOtherTile);
        EXPECT_EQ(
            animationFrameAt(animation, 2 * kDecorPaceTick), kOneTile);
    }

    TEST(
        TileAnimationTest,
        AtlasWithAnimationFrames_CopiesTheFrameAtTheTick)
    {
        auto sheet = sheetOf(Atlas::Floor, 10);

        inkTile(sheet, kOneTile, 40);
        inkTile(sheet, kOtherTile, 200);

        auto flips = withAnimationToggled({}, kOneTile);

        flips = withAnimationFrameAdded(flips, kOneTile);
        flips = withAnimationFrameSet(flips, kOneTile, 1, kOtherTile);

        const auto framedAtlas = atlasWithAnimationFrames(
            sheet, Atlas::Floor, flips, 0);

        EXPECT_EQ(framedAtlas, sheet);

        const auto pacedAtlas = atlasWithAnimationFrames(
            sheet, Atlas::Floor, flips, kDecorPaceTick);
        auto expectedSheet = sheet;

        inkTile(expectedSheet, kOneTile, 200);

        EXPECT_EQ(pacedAtlas, expectedSheet);
    }

    TEST(
        TileAnimationTest,
        AtlasWithAnimationFrames_LeavesTheOtherAtlasAlone)
    {
        auto sheet = sheetOf(Atlas::Wall, 10);
        auto flips = withAnimationToggled({}, kOneTile);

        flips = withAnimationFrameAdded(flips, kOneTile);
        flips = withAnimationFrameSet(flips, kOneTile, 1, kOtherTile);

        EXPECT_EQ(
            atlasWithAnimationFrames(
                sheet, Atlas::Wall, flips, kDecorPaceTick),
            sheet);
    }

}
