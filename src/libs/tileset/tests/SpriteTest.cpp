#include <gtest/gtest.h>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/tileset/PixelClass.hpp>
#include <antwika/tileset/Sprite.hpp>

using antwika::tileset::isBlank;
using antwika::tileset::kMaxFrames;
using antwika::tileset::kOpenSocket;
using antwika::tileset::kSpritePixels;
using antwika::tileset::kSpriteSide;
using antwika::tileset::PixelClass;
using antwika::tileset::Side;
using antwika::tileset::SocketId;
using antwika::tileset::Sprite;
using antwika::tileset::SpriteFrame;
using antwika::tileset::toString;

TEST(SpriteTest, Side_CountsFourSides)
{
    EXPECT_EQ(antwika::enums::kCount<Side>, 4U);
}

TEST(SpriteTest, ToString_NamesEverySide)
{
    EXPECT_EQ(toString(Side::North), "north");
    EXPECT_EQ(toString(Side::East), "east");
    EXPECT_EQ(toString(Side::South), "south");
    EXPECT_EQ(toString(Side::West), "west");
}

TEST(SpriteTest, ToString_FallsBackForAValueThatNamesNoSide)
{
    EXPECT_EQ(toString(static_cast<Side>(42)), "unknown");
}

TEST(SpriteTest, Sprite_KeepsEightByEightFramesThreeSlotsDeep)
{
    EXPECT_EQ(kSpriteSide, 8);
    EXPECT_EQ(kSpritePixels, 64);
    EXPECT_EQ(kMaxFrames, 4);
}

TEST(SpriteTest, Sprite_DefaultsToOneBlankOpenSocketedFrame)
{
    const Sprite sprite;

    EXPECT_EQ(sprite.frameCount, 1);
    EXPECT_EQ(sprite.weight, antwika::tileset::kDefaultWeight);
    EXPECT_TRUE(sprite.on.empty());

    for (const auto socket : sprite.sockets)
    {
        EXPECT_EQ(socket, kOpenSocket);
    }

    for (const auto pixel : sprite.frames[0].pixels)
    {
        EXPECT_EQ(pixel, PixelClass::Blank);
    }
}

TEST(SpriteTest, IsBlank_CallsADefaultConstructedFrameBlank)
{
    EXPECT_TRUE(isBlank(SpriteFrame{}));
}

TEST(SpriteTest, IsBlank_RefusesAFrameHoldingOneInkPixel)
{
    SpriteFrame frame;
    frame.pixels[kSpritePixels - 1] = PixelClass::Ink;

    EXPECT_FALSE(isBlank(frame));
}

TEST(SpriteTest, IsBlank_RefusesAFrameHoldingOnePaperPixel)
{
    SpriteFrame frame;
    frame.pixels[0] = PixelClass::Paper;

    EXPECT_FALSE(isBlank(frame));
}

TEST(SpriteTest, OperatorEquals_TellsApartADifferingPixel)
{
    const Sprite base{.id = 7};
    const auto twin = base;

    auto inked = base;
    inked.frames[0].pixels[0] = PixelClass::Ink;

    EXPECT_EQ(base, twin);
    EXPECT_NE(base, inked);
}

TEST(SpriteTest, OperatorEquals_TellsApartADifferingSocket)
{
    const Sprite base{.id = 7};

    auto resocketed = base;
    resocketed.sockets[1] = SocketId{5};

    EXPECT_NE(base, resocketed);
}

TEST(SpriteTest, OperatorEquals_TellsApartADifferingWeight)
{
    const Sprite base{.id = 7};

    auto reweighted = base;
    reweighted.weight = 9;

    EXPECT_NE(base, reweighted);
}

TEST(SpriteTest, OperatorEquals_TellsApartADifferingId)
{
    const Sprite base{.id = 7};

    auto renamed = base;
    renamed.id = 8;

    EXPECT_NE(base, renamed);
}

TEST(SpriteTest, OperatorEquals_TellsApartADifferingFrameCount)
{
    const Sprite base{.id = 7};

    auto animated = base;
    animated.frameCount = 2;

    EXPECT_NE(base, animated);
}

TEST(SpriteTest, OperatorEquals_TellsApartADifferingOnList)
{
    const Sprite base{.id = 7};

    auto perched = base;
    perched.on.push_back(3);

    EXPECT_NE(base, perched);
}
