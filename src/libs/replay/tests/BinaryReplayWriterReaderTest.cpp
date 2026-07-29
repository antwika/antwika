#include <gtest/gtest.h>

#include <sstream>

#include "antwika/replay/BinaryEventCodec.hpp"
#include "antwika/replay/BinaryReplayReader.hpp"
#include "antwika/replay/BinaryReplayWriter.hpp"
#include "antwika/replay/ReplayFormatError.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::replay::BinaryEventCodec;
using antwika::replay::BinaryReplayReader;
using antwika::replay::BinaryReplayWriter;
using antwika::replay::ReplayFormatError;

namespace
{
    std::vector<TickEvent> roundTrip(const std::vector<TickEvent> &events)
    {
        BinaryEventCodec codec;
        BinaryReplayWriter writer(codec);
        BinaryReplayReader reader(codec);

        std::stringstream stream;
        writer.write(events, stream);
        return reader.read(stream);
    }
} // namespace

TEST(BinaryReplayWriterReaderTest, RoundTripsZeroEvents)
{
    EXPECT_EQ(roundTrip({}), std::vector<TickEvent>{});
}

TEST(BinaryReplayWriterReaderTest, RoundTripsOneEvent)
{
    std::vector<TickEvent> events{
        TickEvent{.tick = 0, .event = Event{.name = "engine.tick"}},
    };
    EXPECT_EQ(roundTrip(events), events);
}

TEST(BinaryReplayWriterReaderTest, RoundTripsManyEventsInOrder)
{
    std::vector<TickEvent> events{
        TickEvent{.tick = 0, .event = Event{.name = "engine.tick"}},
        TickEvent{
            .tick = 0,
            .event = Event{
                .name = "game.score_increment",
                .payload = "amount=1",
            },
        },
        TickEvent{.tick = 1, .event = Event{.name = "engine.tick"}},
        TickEvent{.tick = 2, .event = Event{.name = "engine.tick"}},
        TickEvent{
            .tick = 2,
            .event = Event{
                .name = "game.score_increment",
                .payload = "amount=4",
            },
        },
    };
    EXPECT_EQ(roundTrip(events), events);
}

TEST(BinaryReplayWriterReaderTest, ReadThrowsOnBadMagicBytes)
{
    BinaryEventCodec codec;
    BinaryReplayReader reader(codec);
    std::stringstream stream("NOTA-REPLAY-STREAM");

    EXPECT_THROW((void)reader.read(stream), ReplayFormatError);
}

TEST(BinaryReplayWriterReaderTest, ReadThrowsOnStreamTooShortForMagicBytes)
{
    BinaryEventCodec codec;
    BinaryReplayReader reader(codec);
    std::stringstream stream("AB"); // shorter than the 4 magic bytes themselves

    EXPECT_THROW((void)reader.read(stream), ReplayFormatError);
}

TEST(BinaryReplayWriterReaderTest, ReadThrowsOnUnsupportedVersion)
{
    BinaryEventCodec codec;
    BinaryReplayWriter writer(codec);
    BinaryReplayReader reader(codec);

    std::stringstream stream;
    writer.write({}, stream);

    auto bytes = stream.str();
    // Byte 4 is the most-significant byte of the big-endian version field.
    bytes[4] = static_cast<char>(0xFF);
    std::stringstream corrupted(bytes);

    EXPECT_THROW((void)reader.read(corrupted), ReplayFormatError);
}

TEST(BinaryReplayWriterReaderTest, ReadThrowsRatherThanTrustingBogusCount)
{
    BinaryEventCodec codec;
    BinaryReplayWriter writer(codec);
    BinaryReplayReader reader(codec);

    std::vector<TickEvent> events{
        TickEvent{.tick = 1, .event = Event{.name = "truncated"}},
    };
    std::stringstream stream;
    writer.write(events, stream);

    // Byte 8 is the big-endian event count's most-significant byte.
    // Only one event's worth of bytes actually follows it.
    // Reserving up front on this count would allocate for billions.
    auto bytes = stream.str();
    bytes[8] = static_cast<char>(0xFF);
    bytes[9] = static_cast<char>(0xFF);
    bytes[10] = static_cast<char>(0xFF);
    bytes[11] = static_cast<char>(0xF0);
    std::stringstream corrupted(bytes);

    EXPECT_THROW((void)reader.read(corrupted), ReplayFormatError);
}

TEST(BinaryReplayWriterReaderTest, ReadThrowsOnTruncatedStream)
{
    BinaryEventCodec codec;
    BinaryReplayWriter writer(codec);
    BinaryReplayReader reader(codec);

    std::vector<TickEvent> events{
        TickEvent{.tick = 1, .event = Event{.name = "truncated"}},
    };
    std::stringstream stream;
    writer.write(events, stream);

    auto truncated = stream.str().substr(0, stream.str().size() - 2);
    std::stringstream truncatedStream(truncated);

    EXPECT_THROW((void)reader.read(truncatedStream), ReplayFormatError);
}
