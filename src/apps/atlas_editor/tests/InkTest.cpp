#include <gtest/gtest.h>

#include <cstdint>

#include <antwika/gfx/Color.hpp>

#include "antwika/atlas_editor/Ink.hpp"

using antwika::atlas_editor::inkChannelOf;
using antwika::atlas_editor::kInkChannels;
using antwika::atlas_editor::withInkChannel;
using antwika::gfx::Color;

namespace
{
    constexpr Color kMix{
        .red = 10, .green = 20, .blue = 30, .alpha = 255};
}

TEST(InkTest, InkChannelOf_ReadsEachChannelInTurn)
{
    EXPECT_EQ(10, inkChannelOf(kMix, 0));
    EXPECT_EQ(20, inkChannelOf(kMix, 1));
    EXPECT_EQ(30, inkChannelOf(kMix, 2));
}

TEST(InkTest, InkChannelOf_ReadsBlueBeyondTheLastChannel)
{
    EXPECT_EQ(30, inkChannelOf(kMix, kInkChannels));
}

TEST(InkTest, WithInkChannel_ReplacesTheChannelItNames)
{
    EXPECT_EQ(
        (Color{.red = 7, .green = 20, .blue = 30, .alpha = 255}),
        withInkChannel(kMix, 0, 7));
    EXPECT_EQ(
        (Color{.red = 10, .green = 7, .blue = 30, .alpha = 255}),
        withInkChannel(kMix, 1, 7));
    EXPECT_EQ(
        (Color{.red = 10, .green = 20, .blue = 7, .alpha = 255}),
        withInkChannel(kMix, 2, 7));
}

TEST(InkTest, WithInkChannel_WritesBlueBeyondTheLastChannel)
{
    EXPECT_EQ(
        (Color{.red = 10, .green = 20, .blue = 7, .alpha = 255}),
        withInkChannel(kMix, kInkChannels, 7));
}

TEST(InkTest, WithInkChannel_LeavesTheAlphaAlone)
{
    EXPECT_EQ(255, withInkChannel(kMix, 0, 7).alpha);
}
