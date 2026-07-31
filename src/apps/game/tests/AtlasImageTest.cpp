#include "antwika/game/AtlasImage.hpp"

#include <cstddef>
#include <string>

#include <gtest/gtest.h>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/GfxError.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/game/TileAtlas.hpp"

using antwika::game::kAtlasSize;
using antwika::game::requireAtlasSize;
using antwika::gfx::Bitmap;
using antwika::gfx::GfxError;
using antwika::gfx::Size;

namespace
{
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
    EXPECT_NO_THROW(requireAtlasSize(sized(kAtlasSize)));
}

TEST(AtlasImageTest, RequireAtlasSize_RefusesATooNarrowImage)
{
    EXPECT_THROW(
        requireAtlasSize(sized(
            Size{.width = kAtlasSize.width - 1, .height = kAtlasSize.height})),
        GfxError);
}

TEST(AtlasImageTest, RequireAtlasSize_RefusesATooShortImage)
{
    EXPECT_THROW(
        requireAtlasSize(sized(
            Size{.width = kAtlasSize.width, .height = kAtlasSize.height - 1})),
        GfxError);
}

TEST(AtlasImageTest, RequireAtlasSize_RefusesAnEmptyImage)
{
    EXPECT_THROW(requireAtlasSize(Bitmap{}), GfxError);
}

// The message has to say what to re-export the art as.
// Otherwise it is a louder version of the blank grid it replaces.
TEST(AtlasImageTest, RequireAtlasSize_SaysBothSizesWhenItRefuses)
{
    const auto wrong = Size{.width = 640, .height = 480};

    try
    {
        requireAtlasSize(sized(wrong));
        FAIL() << "a wrong-sized atlas must be refused";
    }
    catch (const GfxError &error)
    {
        const std::string message = error.what();

        EXPECT_NE(message.find("640x480"), std::string::npos) << message;
        EXPECT_NE(
            message.find(
                std::to_string(kAtlasSize.width) + "x"
                + std::to_string(kAtlasSize.height)),
            std::string::npos)
            << message;
    }
}
