#include <gtest/gtest.h>

#include <sstream>

#include "antwika/replay/ReplayReader.hpp"
#include "antwika/replay/ReplayWriter.hpp"
#include "antwika/replay/ReplayFormatError.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::replay::ReplayReader;
using antwika::replay::ReplayWriter;
using antwika::replay::ReplayFormatError;

namespace
{
    std::vector<TickEvent> roundTrip(const std::vector<TickEvent> &events)
    {
        ReplayWriter writer;
        ReplayReader reader;

        std::stringstream stream;
        writer.write(events, stream);
        return reader.read(stream);
    }
} // namespace

TEST(ReplayWriterReaderTest, RoundTripsZeroEvents)
{
    EXPECT_EQ(roundTrip({}), std::vector<TickEvent>{});
}

TEST(ReplayWriterReaderTest, RoundTripsOneEvent)
{
    std::vector<TickEvent> events{
        TickEvent{.tick = 0, .event = Event{.name = "engine.tick"}},
    };
    EXPECT_EQ(roundTrip(events), events);
}

TEST(ReplayWriterReaderTest, RoundTripsManyEventsInOrder)
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

TEST(ReplayWriterReaderTest, ReadThrowsWhenStreamIsNotValidJson)
{
    ReplayReader reader;
    std::stringstream stream("not json");

    EXPECT_THROW((void)reader.read(stream), ReplayFormatError);
}

TEST(ReplayWriterReaderTest, ReadThrowsOnEmptyStream)
{
    ReplayReader reader;
    std::stringstream stream("");

    EXPECT_THROW((void)reader.read(stream), ReplayFormatError);
}

TEST(ReplayWriterReaderTest, ReadThrowsWhenDocumentFailsTheSchema)
{
    ReplayReader reader;
    std::stringstream stream(R"({"magic":"nope","version":1,)"
                              R"("events":[]})");

    EXPECT_THROW((void)reader.read(stream), ReplayFormatError);
}
