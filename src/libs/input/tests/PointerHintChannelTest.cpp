#include <gtest/gtest.h>

#include <optional>

#include "antwika/input/PointerHintChannel.hpp"
#include "antwika/input/PointerHint.hpp"

using antwika::input::PointerHint;
using antwika::input::PointerHintChannel;

TEST(PointerHintChannelTest, ForRenderingOnly_ReportsNothingUntilSomethingIs)
{
    const PointerHintChannel channel;

    EXPECT_EQ(channel.latest(), std::nullopt);
}

TEST(PointerHintChannelTest, ForRenderingOnly_ReportsWhatWasPublished)
{
    PointerHintChannel channel;

    channel.publish(PointerHint{.position = {.x = 7, .y = 9}});

    EXPECT_EQ(
        channel.latest(),
        (PointerHint{.position = {.x = 7, .y = 9}}));
}

TEST(PointerHintChannelTest, ForRenderingOnly_ReportsOnlyTheLatest)
{
    PointerHintChannel channel;

    channel.publish(PointerHint{.position = {.x = 1, .y = 1}});
    channel.publish(PointerHint{.position = {.x = 2, .y = 2}});

    EXPECT_EQ(
        channel.latest(),
        (PointerHint{.position = {.x = 2, .y = 2}}));
}
