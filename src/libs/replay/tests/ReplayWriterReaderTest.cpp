#include <gtest/gtest.h>

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

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

namespace
{
    [[nodiscard]] std::string written(
        const std::vector<TickEvent> &events, ReplayWriter::Layout layout)
    {
        const ReplayWriter writer(layout);

        std::stringstream stream;
        writer.write(events, stream);
        return stream.str();
    }

    [[nodiscard]] std::vector<TickEvent> aSession()
    {
        std::vector<TickEvent> events;
        for (antwika::time::Tick tick = 0; tick < 20; ++tick)
        {
            events.push_back(
                TickEvent{
                    .tick = tick,
                    .event = Event{
                        .name = "input.pointer_move",
                        .payload = R"({"x":512,"y":128})",
                    }});
        }
        return events;
    }
} // namespace

TEST(ReplayWriterReaderTest, WritesOneLineWhenAskedForCompact)
{
    const auto compact =
        written(aSession(), ReplayWriter::Layout::Compact);

    // The trailing newline the writer always adds, and nothing else.
    EXPECT_EQ(std::count(compact.begin(), compact.end(), '\n'), 1);
}

TEST(ReplayWriterReaderTest, CompactIsMuchSmallerThanPretty)
{
    const auto pretty = written(aSession(), ReplayWriter::Layout::Pretty);
    const auto compact =
        written(aSession(), ReplayWriter::Layout::Compact);

    // A third of a pretty-printed recording is whitespace.
    EXPECT_LT(compact.size() * 3, pretty.size() * 2);
}

TEST(ReplayWriterReaderTest, ReadsBackWhatCompactWrote)
{
    const auto events = aSession();
    const ReplayWriter writer(ReplayWriter::Layout::Compact);
    ReplayReader reader;

    std::stringstream stream;
    writer.write(events, stream);

    EXPECT_EQ(reader.read(stream), events);
}
