#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include <antwika/gfx/Size.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/replay/CanvasCheck.hpp"
#include "antwika/replay/ReplayFormatError.hpp"
#include "antwika/replay/ReplayReader.hpp"
#include "antwika/replay/ReplayWriter.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::gfx::Size;
using antwika::log::Level;
using antwika::log::mocks::MockLogger;
using antwika::replay::CanvasCheck;
using antwika::replay::ReplayFormatError;
using antwika::replay::ReplayReader;
using antwika::replay::ReplayWriter;

namespace
{
    std::vector<TickEvent> roundTrip(const std::vector<TickEvent> &events)
    {
        const ReplayWriter writer;
        const ReplayReader reader;

        std::stringstream stream;
        writer.write(events, stream);
        return reader.read(stream);
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

    [[nodiscard]] std::string written(const std::vector<TickEvent> &events)
    {
        const ReplayWriter writer;

        std::stringstream stream;
        writer.write(events, stream);
        return stream.str();
    }

    [[nodiscard]] std::vector<TickEvent> readText(const std::string &text)
    {
        std::stringstream stream(text);
        return ReplayReader().read(stream);
    }
} // namespace

TEST(ReplayWriterReaderTest, RoundTripsZeroEvents)
{
    EXPECT_EQ(roundTrip({}), std::vector<TickEvent>{});
}

TEST(ReplayWriterReaderTest, RoundTripsOneEvent)
{
    const std::vector<TickEvent> events{
        TickEvent{.tick = 0, .event = Event{.name = "engine.tick"}},
    };
    EXPECT_EQ(roundTrip(events), events);
}

TEST(ReplayWriterReaderTest, RoundTripsManyEventsInOrder)
{
    const std::vector<TickEvent> events{
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

// One value a line is the whole format: a header, then a record each.
TEST(ReplayWriterReaderTest, WritesAHeaderLineAndOneLinePerEvent)
{
    const auto text = written(aSession());

    EXPECT_EQ(std::count(text.begin(), text.end(), '\n'), 21);
    EXPECT_EQ(text.find("antwika-replay"), 10U) << text;
}

// A payload holding a newline would end a record early unescaped.
// Which is exactly what one value a line rests on.
TEST(ReplayWriterReaderTest, APayloadWithANewlineStaysOnOneLine)
{
    const std::vector<TickEvent> events{
        TickEvent{
            .tick = 0,
            .event = Event{.name = "a.b", .payload = "one\ntwo"}},
    };
    const auto text = written(events);

    EXPECT_EQ(std::count(text.begin(), text.end(), '\n'), 2);
    EXPECT_EQ(roundTrip(events), events);
}

TEST(ReplayWriterReaderTest, ReadThrowsWhenStreamIsNotValidJson)
{
    EXPECT_THROW(std::ignore = readText("not json"), ReplayFormatError);
}

TEST(ReplayWriterReaderTest, ReadThrowsOnEmptyStream)
{
    EXPECT_THROW(std::ignore = readText(""), ReplayFormatError);
}

TEST(ReplayWriterReaderTest, ReadThrowsWhenTheHeaderFailsTheSchema)
{
    EXPECT_THROW(
        std::ignore = readText(R"({"magic":"nope","version":2})"),
        ReplayFormatError);
}

TEST(ReplayWriterReaderTest, ReadThrowsWhenALineIsNotAJsonValue)
{
    const std::string text =
        R"({"magic":"antwika-replay","version":2})"
        "\n"
        R"({"tick":0,"event":{"name":"a.b","payload":""}})"
        "\n"
        "not json\n";

    try
    {
        std::ignore = readText(text);
        FAIL() << "a line that is not a JSON value should have thrown";
    }
    catch (const ReplayFormatError &error)
    {
        EXPECT_NE(
            std::string(error.what()).find("line 3"), std::string::npos)
            << error.what();
    }
}

// nlohmann's parser is iterative, but copying a value and wording a
// validation failure both recurse per nesting level.
// A crafted line must be refused before anything recursive sees it.
TEST(ReplayWriterReaderTest, ReadRefusesALineNestedPastTheBound)
{
    const std::string text =
        R"({"magic":"antwika-replay","version":2})"
        "\n"
        + std::string(100000, '[') + std::string(100000, ']') + "\n";

    try
    {
        std::ignore = readText(text);
        FAIL() << "a line nested past the bound should have thrown";
    }
    catch (const ReplayFormatError &error)
    {
        const std::string what(error.what());

        EXPECT_NE(what.find("line 2"), std::string::npos) << what;
        EXPECT_NE(what.find("nests deeper"), std::string::npos) << what;
    }
}

// The same refusal when the deep value is the stream's first.
// The whole-document branch would copy it, and a copy recurses.
TEST(ReplayWriterReaderTest, ReadRefusesAnOpeningValueNestedPastTheBound)
{
    const std::string text =
        std::string(100000, '[') + std::string(100000, ']') + "\n";

    try
    {
        std::ignore = readText(text);
        FAIL() << "an opening value nested past the bound should have "
                  "thrown";
    }
    catch (const ReplayFormatError &error)
    {
        EXPECT_NE(
            std::string(error.what()).find("nests deeper"),
            std::string::npos)
            << error.what();
    }
}

// The newline that ends a record says the write got there whole.
// So a last line without one that will not parse was torn off.
// By the kill that ended the run, and what came before it stands.
TEST(ReplayWriterReaderTest, ATornFinalLineIsDroppedRatherThanRefused)
{
    const std::string text =
        R"({"magic":"antwika-replay","version":2})"
        "\n"
        R"({"tick":0,"event":{"name":"a.b","payload":""}})"
        "\n"
        R"({"tick":1,"event":{"name":"a.)";

    EXPECT_EQ(
        readText(text),
        (std::vector<TickEvent>{
            TickEvent{.tick = 0, .event = Event{.name = "a.b"}}}));
}

// The other side of the same kill: between the write and the newline.
// The record itself got there whole, so it is kept rather than dropped.
// Which recordings survive a kill is worth pinning either way.
// A reader wanting the marker first would quietly lose this one.
// This is the test that would say so.
TEST(ReplayWriterReaderTest, ACompleteFinalRecordWithNoNewlineIsKept)
{
    const std::string text =
        R"({"magic":"antwika-replay","version":2})"
        "\n"
        R"({"tick":0,"event":{"name":"a.b","payload":""}})"
        "\n"
        R"({"tick":1,"event":{"name":"c.d","payload":""}})";

    EXPECT_EQ(
        readText(text),
        (std::vector<TickEvent>{
            TickEvent{.tick = 0, .event = Event{.name = "a.b"}},
            TickEvent{.tick = 1, .event = Event{.name = "c.d"}}}));
}

TEST(ReplayWriterReaderTest, BlankLinesBetweenRecordsAreIgnored)
{
    const std::string text =
        R"({"magic":"antwika-replay","version":2})"
        "\n\n   \n"
        R"({"tick":0,"event":{"name":"a.b","payload":""}})"
        "\n\n";

    EXPECT_EQ(readText(text).size(), 1U);
}

TEST(ReplayWriterReaderTest, ARecordThatFailsTheSchemaNamesWhichOne)
{
    const std::string text =
        R"({"magic":"antwika-replay","version":2})"
        "\n"
        R"({"tick":0,"event":{"name":"a.b","payload":""}})"
        "\n"
        R"({"tick":1})"
        "\n";

    try
    {
        std::ignore = readText(text);
        FAIL() << "a record failing the schema should have thrown";
    }
    catch (const ReplayFormatError &error)
    {
        EXPECT_NE(
            std::string(error.what()).find("record 2"),
            std::string::npos)
            << error.what();
    }
}

// The shape every replay was written in before this one.
TEST(ReplayWriterReaderTest, ReadsAWholeDocumentWrittenByAnOlderBuild)
{
    const std::string text =
        R"({"magic":"antwika-replay","version":1,"events":[)"
        R"({"tick":0,"event":{"name":"a.b","payload":"1"}},)"
        R"({"tick":4,"event":{"name":"c.d","payload":"2"}}]})";

    EXPECT_EQ(
        readText(text),
        (std::vector<TickEvent>{
            TickEvent{
                .tick = 0,
                .event = Event{.name = "a.b", .payload = "1"}},
            TickEvent{
                .tick = 4,
                .event = Event{.name = "c.d", .payload = "2"}}}));
}

// Including the indented one every checked-in demo replay used to be.
TEST(ReplayWriterReaderTest, ReadsAPrettyPrintedWholeDocument)
{
    const std::string text = R"({
  "magic": "antwika-replay",
  "version": 1,
  "canvas": {"width": 800, "height": 600},
  "events": [
    {"tick": 2, "event": {"name": "a.b", "payload": ""}}
  ]
})";

    EXPECT_EQ(readText(text).size(), 1U);
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

        const ReplayWriter writer(recorded);
        std::stringstream stream;
        writer.write(aSession(), stream);

        const ReplayReader reader(
            CanvasCheck{.canvas = expected, .logger = logger});
        std::ignore = reader.read(stream);

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
    const ReplayWriter writer(Size{.width = 1024, .height = 640});
    std::stringstream stream;
    writer.write(aSession(), stream);

    const ReplayReader reader(
        CanvasCheck{.canvas = Size{.width = 800, .height = 600}});

    EXPECT_EQ(reader.read(stream), aSession());
}
