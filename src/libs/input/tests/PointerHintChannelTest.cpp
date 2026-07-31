#include "antwika/input/PointerHintChannel.hpp"

#include <gtest/gtest.h>

#include <optional>

#include "antwika/input/PointerHint.hpp"

using antwika::input::PointerHint;
using antwika::input::PointerHintChannel;

TEST(PointerHintChannelTest, ForRenderingOnly_ReportsNothingUntilSomethingIs)
{
    // The state a run is in before the pointer has been seen at all.
    // Distinct from the origin, which is a real place a pointer can be.
    const PointerHintChannel channel;

    EXPECT_EQ(channel.forRenderingOnly(), std::nullopt);
}

TEST(PointerHintChannelTest, ForRenderingOnly_ReportsWhatWasPublished)
{
    PointerHintChannel channel;

    channel.publish(PointerHint{.position = {.x = 7, .y = 9}});

    EXPECT_EQ(
        channel.forRenderingOnly(),
        (PointerHint{.position = {.x = 7, .y = 9}}));
}

TEST(PointerHintChannelTest, ForRenderingOnly_ReportsOnlyTheLatest)
{
    // A hint is where the pointer is, not a log of where it went.
    PointerHintChannel channel;

    channel.publish(PointerHint{.position = {.x = 1, .y = 1}});
    channel.publish(PointerHint{.position = {.x = 2, .y = 2}});

    EXPECT_EQ(
        channel.forRenderingOnly(),
        (PointerHint{.position = {.x = 2, .y = 2}}));
}
