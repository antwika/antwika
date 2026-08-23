#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include <antwika/geometry/Size.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/replay/CanvasCheckOptions.hpp"
#include "antwika/replay/ReplayFormatError.hpp"
#include "antwika/replay/ReplayReader.hpp"
#include "antwika/replay/ReplayWriter.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::geometry::Size;
using antwika::log::Level;
using antwika::log::mocks::MockLogger;
using antwika::replay::CanvasCheckOptions;
using antwika::replay::ReplayFormatError;
using antwika::replay::ReplayReader;
using antwika::replay::ReplayWriter;

namespace
{
    std::vector<TickEvent> getRoundTrip(const std::vector<TickEvent> &events)
    {
        const ReplayWriter writer;
        const ReplayReader reader;

        std::stringstream stream;
        writer.write(events, stream);
        return reader.read(stream);
    }

    [[nodiscard]] std::vector<TickEvent> getASession()
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

    [[nodiscard]] std::string getWritten(const std::vector<TickEvent> &events)
    {
        const ReplayWriter writer;

        std::stringstream stream;
        writer.write(events, stream);
        return stream.str();
    }

    [[nodiscard]] std::vector<TickEvent> getReadText(const std::string &text)
    {
        std::stringstream stream(text);
        return ReplayReader().read(stream);
    }
}

TEST(ReplayWriterReaderTest, Write_RoundTripsZeroEvents)
{
    EXPECT_EQ(getRoundTrip({}), std::vector<TickEvent>{});
}

TEST(ReplayWriterReaderTest, Write_RoundTripsOneEvent)
{
    const std::vector<TickEvent> events{
        TickEvent{.tick = 0, .event = Event{.name = "life.step"}},
    };
    EXPECT_EQ(getRoundTrip(events), events);
}

TEST(ReplayWriterReaderTest, Write_RoundTripsManyEventsInOrder)
{
    const std::vector<TickEvent> events{
        TickEvent{.tick = 0, .event = Event{.name = "life.step"}},
        TickEvent{
            .tick = 0,
            .event = Event{
                .name = "game.score_increment",
                .payload = R"({"amount":1})",
            },
        },
        TickEvent{.tick = 1, .event = Event{.name = "life.step"}},
        TickEvent{.tick = 2, .event = Event{.name = "life.step"}},
        TickEvent{
            .tick = 2,
            .event = Event{
                .name = "game.score_increment",
                .payload = R"({"amount":4})",
            },
        },
    };
    EXPECT_EQ(getRoundTrip(events), events);
}

TEST(ReplayWriterReaderTest, Write_WritesAHeaderAndOneLinePerEvent)
{
    const auto text = getWritten(getASession());

    EXPECT_EQ(std::count(text.begin(), text.end(), '\n'), 21);
    EXPECT_EQ(text.find("antwika-replay"), 10U) << text;
}

TEST(ReplayWriterReaderTest, Write_KeepsANewlinePayloadOnOneLine)
{
    const std::vector<TickEvent> events{
        TickEvent{
            .tick = 0,
            .event = Event{.name = "a.b", .payload = "one\ntwo"}},
    };
    const auto text = getWritten(events);

    EXPECT_EQ(std::count(text.begin(), text.end(), '\n'), 2);
    EXPECT_EQ(getRoundTrip(events), events);
}

TEST(ReplayWriterReaderTest, Read_ThrowsOnAStreamThatIsNotJson)
{
    EXPECT_THROW(std::ignore = getReadText("not json"), ReplayFormatError);
}

TEST(ReplayWriterReaderTest, Read_ThrowsOnAnEmptyStream)
{
    EXPECT_THROW(std::ignore = getReadText(""), ReplayFormatError);
}

TEST(ReplayWriterReaderTest, Read_ThrowsWhenTheHeaderFailsTheSchema)
{
    EXPECT_THROW(
        std::ignore = getReadText(R"({"magic":"nope","version":2})"),
        ReplayFormatError);
}

TEST(ReplayWriterReaderTest, Read_ThrowsOnALineThatIsNotJson)
{
    const std::string text =
        R"({"magic":"antwika-replay","version":2})"
        "\n"
        R"({"tick":0,"event":{"name":"a.b","payload":""}})"
        "\n"
        "not json\n";

    try
    {
        std::ignore = getReadText(text);
        FAIL() << "a line that is not a JSON value should have thrown";
    }
    catch (const ReplayFormatError &error)
    {
        EXPECT_NE(
            std::string(error.what()).find("line 3"), std::string::npos)
            << error.what();
    }
}

TEST(ReplayWriterReaderTest, Read_RefusesALineNestedPastTheBound)
{
    const std::string text =
        R"({"magic":"antwika-replay","version":2})"
        "\n"
        + std::string(100000, '[') + std::string(100000, ']') + "\n";

    try
    {
        std::ignore = getReadText(text);
        FAIL() << "a line nested past the bound should have thrown";
    }
    catch (const ReplayFormatError &error)
    {
        const std::string what(error.what());

        EXPECT_NE(what.find("line 2"), std::string::npos) << what;
        EXPECT_NE(what.find("nests deeper"), std::string::npos) << what;
    }
}

TEST(ReplayWriterReaderTest, Read_RefusesAnOpeningValueTooDeep)
{
    const std::string text =
        std::string(100000, '[') + std::string(100000, ']') + "\n";

    try
    {
        std::ignore = getReadText(text);
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

TEST(ReplayWriterReaderTest, Read_DropsATornFinalLine)
{
    const std::string text =
        R"({"magic":"antwika-replay","version":2})"
        "\n"
        R"({"tick":0,"event":{"name":"a.b","payload":""}})"
        "\n"
        R"({"tick":1,"event":{"name":"a.)";

    EXPECT_EQ(
        getReadText(text),
        (std::vector<TickEvent>{
            TickEvent{.tick = 0, .event = Event{.name = "a.b"}}}));
}

TEST(ReplayWriterReaderTest, Read_KeepsAFinalRecordWithNoNewline)
{
    const std::string text =
        R"({"magic":"antwika-replay","version":2})"
        "\n"
        R"({"tick":0,"event":{"name":"a.b","payload":""}})"
        "\n"
        R"({"tick":1,"event":{"name":"c.d","payload":""}})";

    EXPECT_EQ(
        getReadText(text),
        (std::vector<TickEvent>{
            TickEvent{.tick = 0, .event = Event{.name = "a.b"}},
            TickEvent{.tick = 1, .event = Event{.name = "c.d"}}}));
}

TEST(ReplayWriterReaderTest, Read_IgnoresBlankLinesBetweenRecords)
{
    const std::string text =
        R"({"magic":"antwika-replay","version":2})"
        "\n\n   \n\t\n"
        R"({"tick":0,"event":{"name":"a.b","payload":""}})"
        "\n\n";

    EXPECT_EQ(getReadText(text).size(), 1U);
}

TEST(ReplayWriterReaderTest, Read_NamesTheRecordThatFailsTheSchema)
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
        std::ignore = getReadText(text);
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

TEST(ReplayWriterReaderTest, Read_TakesAnOlderBuildsWholeDocument)
{
    const std::string text =
        R"({"magic":"antwika-replay","version":1,"events":[)"
        R"({"tick":0,"event":{"name":"a.b","payload":"1"}},)"
        R"({"tick":4,"event":{"name":"c.d","payload":"2"}}]})";

    EXPECT_EQ(
        getReadText(text),
        (std::vector<TickEvent>{
            TickEvent{
                .tick = 0,
                .event = Event{.name = "a.b", .payload = "1"}},
            TickEvent{
                .tick = 4,
                .event = Event{.name = "c.d", .payload = "2"}}}));
}

TEST(ReplayWriterReaderTest, Read_RefusesContentAfterAWholeDocument)
{
    const std::string document =
        R"({"magic":"antwika-replay","version":1,"events":[)"
        R"({"tick":0,"event":{"name":"a.b","payload":""}}]})";

    try
    {
        std::ignore = getReadText(document + "\n" + document + "\n");
        FAIL() << "a second recording should have thrown";
    }
    catch (const ReplayFormatError &error)
    {
        EXPECT_NE(
            std::string(error.what()).find("second header"),
            std::string::npos)
            << error.what();
    }
}

TEST(ReplayWriterReaderTest, Read_NamesTheVersionWhenContentFollows)
{
    const std::string document =
        R"({"magic":"antwika-replay","version":1,"events":[)"
        R"({"tick":0,"event":{"name":"a.b","payload":""}}]})";

    try
    {
        std::ignore = getReadText(document + "\ntrailing\n");
        FAIL() << "content after a whole document should have thrown";
    }
    catch (const ReplayFormatError &error)
    {
        const std::string what(error.what());

        EXPECT_NE(what.find("content follows"), std::string::npos) << what;
        EXPECT_NE(what.find("version 1 document"), std::string::npos)
            << what;
    }
}

TEST(ReplayWriterReaderTest, Read_LoadsAHeaderWithAnUnknownMember)
{
    const std::string text =
        R"({"magic":"antwika-replay","version":2,"novel":true})"
        "\n"
        R"({"tick":0,"event":{"name":"a.b","payload":""}})"
        "\n";

    EXPECT_EQ(getReadText(text).size(), 1U);
}

TEST(ReplayWriterReaderTest, Read_RefusesAHandCraftedEngineTick)
{
    const std::string text =
        R"({"magic":"antwika-replay","version":2})"
        "\n"
        R"({"tick":0,"event":{"name":"engine.tick","payload":""}})"
        "\n";

    try
    {
        std::ignore = getReadText(text);
        FAIL() << "an engine.tick record should have thrown";
    }
    catch (const ReplayFormatError &error)
    {
        EXPECT_NE(
            std::string(error.what()).find("engine.tick"),
            std::string::npos)
            << error.what();
    }
}

TEST(ReplayWriterReaderTest, Read_TakesCarriageReturnLineEndings)
{
    const std::string text =
        R"({"magic":"antwika-replay","version":2})"
        "\r\n"
        R"({"tick":0,"event":{"name":"a.b","payload":""}})"
        "\r\n";

    EXPECT_EQ(getReadText(text).size(), 1U);
}

TEST(ReplayWriterReaderTest, Read_TakesALeadingByteOrderMark)
{
    const std::string text =
        "\xEF\xBB\xBF"
        R"({"magic":"antwika-replay","version":2})"
        "\n"
        R"({"tick":0,"event":{"name":"a.b","payload":""}})"
        "\n";

    EXPECT_EQ(getReadText(text).size(), 1U);
}

TEST(ReplayWriterReaderTest, Read_TakesAPrettyPrintedWholeDocument)
{
    const std::string text = R"({
  "magic": "antwika-replay",
  "version": 1,
  "canvas": {"width": 800, "height": 600},
  "events": [
    {"tick": 2, "event": {"name": "a.b", "payload": ""}}
  ]
})";

    EXPECT_EQ(getReadText(text).size(), 1U);
}

namespace
{
    [[nodiscard]] std::vector<std::string> getWarningsFromRoundTrip(
        std::optional<Size> recordedSize, std::optional<Size> expectedSize)
    {
        std::vector<std::string> warnings;
        MockLogger logger;
        EXPECT_CALL(logger, log(Level::Warning, testing::_))
            .WillRepeatedly(
                [&warnings](Level, std::string_view message)
                {
                    warnings.emplace_back(message);
                });

        const ReplayWriter writer(recordedSize);
        std::stringstream stream;
        writer.write(getASession(), stream);

        const ReplayReader reader(
            CanvasCheckOptions{.canvasSize = expectedSize, .logger = logger});
        std::ignore = reader.read(stream);

        return warnings;
    }
}

TEST(ReplayWriterReaderTest, Read_WarnsOnACanvasMismatch)
{
    const auto warnings = getWarningsFromRoundTrip(
        Size{.width = 1024, .height = 640},
        Size{.width = 800, .height = 600});

    ASSERT_EQ(warnings.size(), 1U);
    EXPECT_NE(warnings.front().find("1024x640"), std::string::npos)
        << warnings.front();
    EXPECT_NE(warnings.front().find("800x600"), std::string::npos)
        << warnings.front();
}

TEST(ReplayWriterReaderTest, Read_SaysNothingWhenTheCanvasesMatch)
{
    EXPECT_TRUE(
        getWarningsFromRoundTrip(
            Size{.width = 1024, .height = 640},
            Size{.width = 1024, .height = 640})
            .empty());
}

TEST(ReplayWriterReaderTest, Read_SaysNothingWithNoRecordedCanvas)
{
    EXPECT_TRUE(
        getWarningsFromRoundTrip(
            std::nullopt, Size{.width = 800, .height = 600})
            .empty());
}

TEST(ReplayWriterReaderTest, Read_SaysNothingWhenTheCallerClaimsNone)
{
    EXPECT_TRUE(
        getWarningsFromRoundTrip(
            Size{.width = 1024, .height = 640}, std::nullopt)
            .empty());
}

TEST(ReplayWriterReaderTest, Read_HandlesAMismatchWithNoLogger)
{
    const ReplayWriter writer(Size{.width = 1024, .height = 640});
    std::stringstream stream;
    writer.write(getASession(), stream);

    const ReplayReader reader(
        CanvasCheckOptions{.canvasSize = Size{.width = 800, .height = 600}});

    EXPECT_EQ(reader.read(stream), getASession());
}
