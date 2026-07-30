#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/gfx/Size.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/replay/CanvasCheck.hpp"
#include "antwika/replay/ReplayReader.hpp"
#include "antwika/replay/ReplayWriter.hpp"
#include "antwika/replay/ReplayFormatError.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::gfx::Size;
using antwika::log::Level;
using antwika::log::mocks::MockLogger;
using antwika::replay::CanvasCheck;
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

namespace
{
    // Writes a session against `recorded`.
    // Reads it back against `expected`.
    // Returns whatever the reader had to say about the difference.
    [[nodiscard]] std::vector<std::string> warningsFromRoundTrip(
        std::optional<Size> recorded, std::optional<Size> expected)
    {
        std::vector<std::string> warnings;
        MockLogger logger;
        EXPECT_CALL(logger, log(Level::Warning, testing::_))
            .WillRepeatedly(
                [&warnings](Level, std::string_view message)
                {
                    warnings.emplace_back(message);
                });

        const ReplayWriter writer(
            ReplayWriter::Layout::Compact, recorded);
        std::stringstream stream;
        writer.write(aSession(), stream);

        const ReplayReader reader(
            CanvasCheck{.canvas = expected, .logger = logger});
        (void)reader.read(stream);

        return warnings;
    }
} // namespace

TEST(ReplayWriterReaderTest, WarnsWhenTheRecordedCanvasIsNotTheOneInUse)
{
    const auto warnings = warningsFromRoundTrip(
        Size{.width = 1024, .height = 640},
        Size{.width = 800, .height = 600});

    ASSERT_EQ(warnings.size(), 1U);
    EXPECT_NE(warnings.front().find("1024x640"), std::string::npos)
        << warnings.front();
    EXPECT_NE(warnings.front().find("800x600"), std::string::npos)
        << warnings.front();
}

TEST(ReplayWriterReaderTest, SaysNothingWhenTheCanvasesMatch)
{
    EXPECT_TRUE(
        warningsFromRoundTrip(
            Size{.width = 1024, .height = 640},
            Size{.width = 1024, .height = 640})
            .empty());
}

// A recording written before the field says nothing about its canvas.
// It is taken at face value rather than complained about.
TEST(ReplayWriterReaderTest, SaysNothingWhenTheRecordingHasNoCanvas)
{
    EXPECT_TRUE(
        warningsFromRoundTrip(
            std::nullopt, Size{.width = 800, .height = 600})
            .empty());
}

TEST(ReplayWriterReaderTest, SaysNothingWhenTheCallerClaimsNoCanvas)
{
    EXPECT_TRUE(
        warningsFromRoundTrip(
            Size{.width = 1024, .height = 640}, std::nullopt)
            .empty());
}

// A check with nowhere to report to is no reason to crash.
// Nor to reach for a logger of the library's own.
TEST(ReplayWriterReaderTest, MismatchWithNoLoggerIsReadNormally)
{
    const ReplayWriter writer(
        ReplayWriter::Layout::Compact, Size{.width = 1024, .height = 640});
    std::stringstream stream;
    writer.write(aSession(), stream);

    const ReplayReader reader(
        CanvasCheck{.canvas = Size{.width = 800, .height = 600}});

    EXPECT_EQ(reader.read(stream), aSession());
}
