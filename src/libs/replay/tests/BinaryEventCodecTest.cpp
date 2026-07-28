#include <gtest/gtest.h>

#include <limits>
#include <sstream>

#include "antwika/replay/BinaryEventCodec.hpp"

using antwika::event::Event;
using antwika::event::TimedEvent;
using antwika::replay::BinaryEventCodec;

namespace
{
    TimedEvent roundTrip(const TimedEvent &event)
    {
        BinaryEventCodec codec;
        std::stringstream stream;
        codec.encode(event, stream);
        return codec.decode(stream);
    }
} // namespace

TEST(BinaryEventCodecTest, RoundTripsATypicalEvent)
{
    TimedEvent event{.tick = 42, .event = Event{.name = "game.score_increment", .payload = "amount=5"}};
    EXPECT_EQ(roundTrip(event), event);
}

TEST(BinaryEventCodecTest, RoundTripsEmptyNameAndPayload)
{
    TimedEvent event{.tick = 0, .event = Event{}};
    EXPECT_EQ(roundTrip(event), event);
}

TEST(BinaryEventCodecTest, RoundTripsNonAsciiUtf8Bytes)
{
    TimedEvent event{.tick = 1, .event = Event{.name = "événement", .payload = "payload-\xc3\xa9"}};
    EXPECT_EQ(roundTrip(event), event);
}

TEST(BinaryEventCodecTest, RoundTripsMaxTickValue)
{
    TimedEvent event{.tick = std::numeric_limits<antwika::time::Tick>::max(), .event = Event{.name = "max-tick"}};
    EXPECT_EQ(roundTrip(event), event);
}

TEST(BinaryEventCodecTest, DecodeThrowsWhenTickFieldIsTruncated)
{
    BinaryEventCodec codec;
    std::stringstream stream;
    codec.encode(TimedEvent{.tick = 1, .event = Event{.name = "truncated"}}, stream);

    // Cut off inside the 8-byte tick field itself.
    // That differs from DecodeThrowsOnTruncatedStream below.
    // That one truncates later, inside a length-prefixed string.
    // This exercises readU64's own truncation check, not just readU32's.
    auto truncated = stream.str().substr(0, 4);
    std::stringstream truncatedStream(truncated);

    EXPECT_THROW((void)codec.decode(truncatedStream), std::runtime_error);
}

TEST(BinaryEventCodecTest, DecodeThrowsOnTruncatedStream)
{
    BinaryEventCodec codec;
    std::stringstream stream;
    codec.encode(TimedEvent{.tick = 1, .event = Event{.name = "truncated"}}, stream);

    auto truncated = stream.str().substr(0, stream.str().size() - 1);
    std::stringstream truncatedStream(truncated);

    EXPECT_THROW((void)codec.decode(truncatedStream), std::runtime_error);
}

TEST(BinaryEventCodecTest, DecodeThrowsWhenStringContentIsTruncated)
{
    BinaryEventCodec codec;
    std::stringstream stream;
    codec.encode(TimedEvent{.tick = 1, .event = Event{.name = "hello"}}, stream);

    // Keep the tick (8 bytes) and the name's length prefix (4 bytes) intact.
    // Cut off partway through the name's content bytes instead.
    // DecodeThrowsOnTruncatedStream above truncates a length field itself.
    // This exercises readString's other branch instead.
    // There, the length was readable but the content wasn't.
    auto truncated = stream.str().substr(0, 8 + 4 + 2);
    std::stringstream truncatedStream(truncated);

    EXPECT_THROW((void)codec.decode(truncatedStream), std::runtime_error);
}
