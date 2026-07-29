#include <gtest/gtest.h>

#include <sstream>

#include "antwika/replay/JsonReplayReader.hpp"
#include "antwika/replay/JsonReplayWriter.hpp"
#include "antwika/replay/ReplayFormatError.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::replay::JsonReplayReader;
using antwika::replay::JsonReplayWriter;
using antwika::replay::ReplayFormatError;

namespace
{
    std::vector<TickEvent> roundTrip(const std::vector<TickEvent> &events)
    {
        JsonReplayWriter writer;
        JsonReplayReader reader;

        std::stringstream stream;
        writer.write(events, stream);
        return reader.read(stream);
    }
} // namespace

TEST(JsonReplayWriterReaderTest, RoundTripsZeroEvents)
{
    EXPECT_EQ(roundTrip({}), std::vector<TickEvent>{});
}

TEST(JsonReplayWriterReaderTest, RoundTripsOneEvent)
{
    std::vector<TickEvent> events{
        TickEvent{.tick = 0, .event = Event{.name = "engine.tick"}},
    };
    EXPECT_EQ(roundTrip(events), events);
}

TEST(JsonReplayWriterReaderTest, RoundTripsManyEventsInOrder)
{
    std::vector<TickEvent> events{
        TickEvent{.tick = 0, .event = Event{.name = "engine.tick"}},
        TickEvent{
            .tick = 0,
            .event = Event{
                .name = "game.score_increment",
                .payload = R"({"amount":1})",
            },
        },
        TickEvent{.tick = 1, .event = Event{.name = "engine.tick"}},
        TickEvent{.tick = 2, .event = Event{.name = "engine.tick"}},
        TickEvent{
            .tick = 2,
            .event = Event{
                .name = "game.score_increment",
                .payload = R"({"amount":4})",
            },
        },
    };
    EXPECT_EQ(roundTrip(events), events);
}

TEST(JsonReplayWriterReaderTest, ReadThrowsWhenStreamIsNotValidJson)
{
    JsonReplayReader reader;
    std::stringstream stream("not json");

    EXPECT_THROW((void)reader.read(stream), ReplayFormatError);
}

TEST(JsonReplayWriterReaderTest, ReadThrowsOnEmptyStream)
{
    JsonReplayReader reader;
    std::stringstream stream("");

    EXPECT_THROW((void)reader.read(stream), ReplayFormatError);
}

TEST(JsonReplayWriterReaderTest, ReadThrowsWhenDocumentFailsTheSchema)
{
    JsonReplayReader reader;
    std::stringstream stream(R"({"magic":"nope","version":1,)"
                              R"("events":[]})");

    EXPECT_THROW((void)reader.read(stream), ReplayFormatError);
}
