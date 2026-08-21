#include <gtest/gtest.h>

#include <array>
#include <span>
#include <vector>

#include "antwika/sound/SampleBuffer.hpp"

using antwika::sound::SampleBuffer;

namespace
{
    struct Planes final
    {
        std::vector<std::vector<float>> data;
        std::vector<std::span<float>> views;

        explicit Planes(std::size_t channels, std::size_t frames)
            : data(channels, std::vector<float>(frames, 1.0F))
        {
            for (auto &plane : data)
            {
                views.emplace_back(plane);
            }
        }

        [[nodiscard]] SampleBuffer buffer(std::size_t frames)
        {
            return SampleBuffer{.channels = views, .frames = frames};
        }
    };
}

TEST(SampleBufferTest, ChannelCount_IsHowManySpansItHolds)
{
    Planes planes(2, 4);

    EXPECT_EQ(planes.buffer(4).channelCount(), 2U);
}

TEST(SampleBufferTest, IsValid_TakesSpansLongEnoughForTheFrames)
{
    Planes planes(2, 8);

    EXPECT_TRUE(planes.buffer(8).isValid());
    EXPECT_TRUE(planes.buffer(4).isValid());
}

TEST(SampleBufferTest, IsValid_RefusesASpanShorterThanTheFrames)
{
    Planes planes(2, 4);

    EXPECT_FALSE(planes.buffer(8).isValid());
}

TEST(SampleBufferTest, IsValid_RefusesNoChannelsAtAll)
{
    const SampleBuffer emptyBuffer{.channels = {}, .frames = 4};

    EXPECT_FALSE(emptyBuffer.isValid());
}

TEST(SampleBufferTest, IsValid_RefusesMoreChannelsThanAreAllowed)
{
    Planes planes(antwika::sound::kMaxChannels + 1, 4);

    EXPECT_FALSE(planes.buffer(4).isValid());
}

TEST(SampleBufferTest, Silence_ClearsExactlyTheFramesInPlay)
{
    Planes planes(2, 8);

    planes.buffer(4).silence();

    EXPECT_EQ(planes.data[0][0], 0.0F);
    EXPECT_EQ(planes.data[0][3], 0.0F);
    EXPECT_EQ(planes.data[0][4], 1.0F);
    EXPECT_EQ(planes.data[1][0], 0.0F);
}
