#include "antwika/game/AtlasImage.hpp"

#include <array>
#include <cstddef>
#include <string>

#include <gtest/gtest.h>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/GfxError.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/game/TileAtlas.hpp"

using antwika::game::AtlasKind;
using antwika::game::atlasSizeOf;
using antwika::game::requireAtlasSize;
using antwika::gfx::Bitmap;
using antwika::gfx::GfxError;
using antwika::gfx::Size;

namespace
{
    // Every sheet, so a table-driven test cannot forget one.
    constexpr std::array<AtlasKind, antwika::game::kAtlasKindCount>
        kEverySheet{
            AtlasKind::OneByOne,
            AtlasKind::TwoByTwo,
            AtlasKind::ThreeByThree};

    // Built rather than loaded.
    // So every answer here is reachable with no image on disk.
    [[nodiscard]] Bitmap sized(Size size)
    {
        const auto bytes = static_cast<std::size_t>(size.width)
            * static_cast<std::size_t>(size.height)
            * antwika::gfx::kBytesPerPixel;

        return Bitmap{.size = size, .pixels = std::vector<std::uint8_t>(bytes)};
    }
} // namespace

TEST(AtlasImageTest, RequireAtlasSize_AcceptsTheSizeTheHeaderAddresses)
{
    for (const auto kind : kEverySheet)
    {
        EXPECT_NO_THROW(
            requireAtlasSize(sized(atlasSizeOf(kind)), kind, "sheet.png"));
    }
}

TEST(AtlasImageTest, RequireAtlasSize_RefusesATooNarrowImage)
{
    for (const auto kind : kEverySheet)
    {
        const auto expected = atlasSizeOf(kind);

        EXPECT_THROW(
            requireAtlasSize(
                sized(Size{
                    .width = expected.width - 1,
                    .height = expected.height}),
                kind,
                "sheet.png"),
            GfxError);
    }
}

TEST(AtlasImageTest, RequireAtlasSize_RefusesATooShortImage)
{
    for (const auto kind : kEverySheet)
    {
        const auto expected = atlasSizeOf(kind);

        EXPECT_THROW(
            requireAtlasSize(
                sized(Size{
                    .width = expected.width,
                    .height = expected.height - 1}),
                kind,
                "sheet.png"),
            GfxError);
    }
}

TEST(AtlasImageTest, RequireAtlasSize_RefusesAnEmptyImage)
{
    EXPECT_THROW(
        requireAtlasSize(Bitmap{}, AtlasKind::OneByOne, "sheet.png"),
        GfxError);
}

// One sheet's size must not excuse another's file.
// The three are distinct, so a swapped pair of paths must be refused.
TEST(AtlasImageTest, RequireAtlasSize_RefusesAnotherSheetsSize)
{
    EXPECT_THROW(
        requireAtlasSize(
            sized(atlasSizeOf(AtlasKind::TwoByTwo)),
            AtlasKind::OneByOne,
            "atlas_1x1.png"),
        GfxError);
}

// The message has to say what to re-export the art as.
// Otherwise it is a louder version of the blank grid it replaces.
// And it has to name the file, there being three it could be.
TEST(AtlasImageTest, RequireAtlasSize_SaysTheFileAndBothSizesWhenItRefuses)
{
    const auto wrong = Size{.width = 640, .height = 480};
    const auto expected = atlasSizeOf(AtlasKind::TwoByTwo);

    try
    {
        requireAtlasSize(
            sized(wrong), AtlasKind::TwoByTwo, "atlas_2x2.png");
        FAIL() << "a wrong-sized atlas must be refused";
    }
    catch (const GfxError &error)
    {
        const std::string message = error.what();

        EXPECT_NE(message.find("atlas_2x2.png"), std::string::npos)
            << message;
        EXPECT_NE(message.find("640x480"), std::string::npos) << message;
        EXPECT_NE(
            message.find(
                std::to_string(expected.width) + "x"
                + std::to_string(expected.height)),
            std::string::npos)
            << message;
    }
}
