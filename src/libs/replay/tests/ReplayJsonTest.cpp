#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <string>
#include <tuple>
#include <vector>

#include <antwika/gfx/Size.hpp>

#include "antwika/replay/EventJson.hpp"
#include "antwika/replay/ReplayDocument.hpp"
#include "antwika/replay/ReplayFormatError.hpp"
#include "antwika/replay/ReplayHeader.hpp"
#include "antwika/replay/ReplayJson.hpp"
#include "antwika/replay/ReplayMigrations.hpp"
#include "antwika/replay/SchemaVersion.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::gfx::Size;
using antwika::replay::kReplayDocumentVersion;
using antwika::replay::replayFromJson;
using antwika::replay::replayHeaderFromJson;
using antwika::replay::replayHeaderToJson;
using antwika::replay::replayRecordsFromJson;
using antwika::replay::ReplayDocument;
using antwika::replay::ReplayFormatError;
using antwika::replay::ReplayHeader;
using antwika::replay::standardReplayMigrations;

namespace
{
    // The chain is injected rather than reached for.
    // Which is exactly what ReplayReader does with its own.
    ReplayDocument read(const nlohmann::json &document)
    {
        return replayFromJson(document, standardReplayMigrations());
    }

    ReplayHeader readHeader(const nlohmann::json &header)
    {
        return replayHeaderFromJson(header, standardReplayMigrations());
    }

    std::vector<TickEvent> readRecords(const nlohmann::json &records)
    {
        return replayRecordsFromJson(
            records, kReplayDocumentVersion, standardReplayMigrations());
    }

    nlohmann::json aRecord(const int tick, const std::string &name)
    {
        return nlohmann::json{
            {"tick", tick},
            {"event", nlohmann::json{{"name", name}, {"payload", ""}}}};
    }
} // namespace

TEST(ReplayJsonTest, HeaderRoundTripsThroughItsOwnEncoding)
{
    const ReplayHeader header{
        .version = kReplayDocumentVersion,
        .canvas = Size{.width = 1024, .height = 640}};

    EXPECT_EQ(readHeader(replayHeaderToJson(header)), header);
}

TEST(ReplayJsonTest, HeaderSaysWhichFormatAndWhichVersionItIs)
{
    const auto encoded = replayHeaderToJson(ReplayHeader{});

    EXPECT_EQ(encoded.at("magic"), "antwika-replay");
    EXPECT_EQ(encoded.at("version"), kReplayDocumentVersion);
}

TEST(ReplayJsonTest, HeaderWritesNoCanvasWhenTheRunClaimedNone)
{
    EXPECT_FALSE(replayHeaderToJson(ReplayHeader{}).contains("canvas"));
}

TEST(ReplayJsonTest, HeaderIsOneLineWhenDumped)
{
    const auto text = replayHeaderToJson(ReplayHeader{}).dump();

    EXPECT_EQ(text.find('\n'), std::string::npos) << text;
}

TEST(ReplayJsonTest, HeaderThrowsOnBadMagic)
{
    EXPECT_THROW(
        std::ignore = readHeader(nlohmann::json{{"magic", "nope"}}),
        ReplayFormatError);
}

TEST(ReplayJsonTest, HeaderThrowsWhenMagicIsMissing)
{
    EXPECT_THROW(
        std::ignore = readHeader(nlohmann::json{{"version", 2}}),
        ReplayFormatError);
}

TEST(ReplayJsonTest, HeaderThrowsWhenTheCanvasIsMalformed)
{
    EXPECT_THROW(
        std::ignore = readHeader(nlohmann::json{
            {"magic", "antwika-replay"},
            {"canvas", nlohmann::json{{"width", 1024}}}}),
        ReplayFormatError);
}

// The decode is get<std::uint32_t>(), which narrows in silence.
// So 4294967297 used to validate and read back as 1.
// The schema states the bound instead of the decode checking after.
TEST(ReplayJsonTest, HeaderThrowsWhenACanvasExtentIsWiderThanUint32)
{
    EXPECT_THROW(
        std::ignore = readHeader(nlohmann::json{
            {"magic", "antwika-replay"},
            {"canvas",
             nlohmann::json{
                 {"width", std::int64_t{4294967297}}, {"height", 640}}}}),
        ReplayFormatError);
}

// The largest a canvas may state is still read, unchanged.
TEST(ReplayJsonTest, HeaderReadsACanvasExtentAtTheUint32Maximum)
{
    const ReplayHeader header = readHeader(nlohmann::json{
        {"magic", "antwika-replay"},
        {"canvas",
         nlohmann::json{
             {"width", std::numeric_limits<std::uint32_t>::max()},
             {"height", 640}}}});

    ASSERT_TRUE(header.canvas.has_value());
    EXPECT_EQ(
        header.canvas->width, std::numeric_limits<std::uint32_t>::max());
}

// docs/schema-versioning.md: the header only grows additively.
// A member this build never heard of is a younger release's addition.
// So it is passed over rather than refused.
// Refusing unknown members broke pre-canvas builds when "canvas" arrived.
TEST(ReplayJsonTest, HeaderPassesOverAMemberItDoesNotKnow)
{
    const auto header = readHeader(
        nlohmann::json{{"magic", "antwika-replay"}, {"novel", 4}});

    EXPECT_EQ(header.version, 1U);
}

TEST(ReplayJsonTest, RecordsRoundTripInOrder)
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
        TickEvent{.tick = 2, .event = Event{.name = "life.step"}},
    };

    EXPECT_EQ(readRecords(nlohmann::json(events)), events);
}

TEST(ReplayJsonTest, RecordsThrowWhenTheyAreNotASequence)
{
    EXPECT_THROW(
        std::ignore = readRecords(nlohmann::json::object()),
        ReplayFormatError);
}

TEST(ReplayJsonTest, RecordsThrowWhenOneIsMalformed)
{
    EXPECT_THROW(
        std::ignore = readRecords(
            nlohmann::json::array({nlohmann::json{{"tick", 0}}})),
        ReplayFormatError);
}

TEST(ReplayJsonTest, RecordsThrowWhenOneIsNotAnObjectAtAll)
{
    EXPECT_THROW(
        std::ignore = readRecords(nlohmann::json::array({42})),
        ReplayFormatError);
}

// The first of the two rules a whole document stated by being one.
// Neither has anywhere to live in a per-line schema.
TEST(ReplayJsonTest, RecordsThrowWhenATickGoesBackwards)
{
    const auto records =
        nlohmann::json::array({aRecord(4, "a.b"), aRecord(1, "a.b")});

    try
    {
        std::ignore = readRecords(records);
        FAIL() << "a tick going backwards should have been refused";
    }
    catch (const ReplayFormatError &error)
    {
        const std::string message = error.what();
        EXPECT_NE(message.find("record 2"), std::string::npos) << message;
        EXPECT_NE(message.find("tick 1"), std::string::npos) << message;
        EXPECT_NE(message.find("tick 4"), std::string::npos) << message;
    }
}

TEST(ReplayJsonTest, RecordsAcceptSeveralOnOneTick)
{
    const auto records = nlohmann::json::array(
        {aRecord(4, "a.b"), aRecord(4, "c.d"), aRecord(5, "e.f")});

    EXPECT_EQ(readRecords(records).size(), 3U);
}

// The second of them.
// Two recordings in one file would replay as a single session.
// With the second one's ticks starting over.
TEST(ReplayJsonTest, RecordsThrowOnASecondHeaderPartWayThrough)
{
    const auto records = nlohmann::json::array(
        {aRecord(0, "a.b"), replayHeaderToJson(ReplayHeader{})});

    try
    {
        std::ignore = readRecords(records);
        FAIL() << "a second header should have been refused";
    }
    catch (const ReplayFormatError &error)
    {
        const std::string message = error.what();
        EXPECT_NE(message.find("record 2"), std::string::npos) << message;
        EXPECT_NE(message.find("second header"), std::string::npos)
            << message;
    }
}

TEST(ReplayJsonTest, WholeDocumentReadsAsAHeaderAndItsRecords)
{
    const std::vector<TickEvent> events{
        TickEvent{
            .tick = 1,
            .event = Event{.name = "life.toggle_cell", .payload = "{}"}},
    };

    const auto document = read(nlohmann::json{
        {"magic", "antwika-replay"},
        {"version", 1},
        {"events", nlohmann::json(events)},
    });

    EXPECT_EQ(document, (ReplayDocument{.events = events}));
}

TEST(ReplayJsonTest, WholeDocumentKeepsTheCanvasItWasRecordedAgainst)
{
    const auto document = read(nlohmann::json{
        {"magic", "antwika-replay"},
        {"version", 1},
        {"events", nlohmann::json::array()},
        {"canvas", nlohmann::json{{"width", 1024}, {"height", 640}}},
    });

    EXPECT_EQ(document.canvas, (Size{.width = 1024, .height = 640}));
}

TEST(ReplayJsonTest, WholeDocumentThrowsWhenInputIsNotAnObject)
{
    EXPECT_THROW(
        std::ignore = read(nlohmann::json::array()), ReplayFormatError);
}

TEST(ReplayJsonTest, WholeDocumentThrowsWhenMagicFieldIsMissing)
{
    EXPECT_THROW(
        std::ignore = read(nlohmann::json{
            {"version", 1},
            {"events", nlohmann::json::array()},
        }),
        ReplayFormatError);
}

TEST(ReplayJsonTest, WholeDocumentThrowsWhenEventsFieldIsNotAnArray)
{
    EXPECT_THROW(
        std::ignore = read(nlohmann::json{
            {"magic", "antwika-replay"},
            {"version", 1},
            {"events", "not an array"},
        }),
        ReplayFormatError);
}

// The whole point of the field being optional.
// Every replay checked in before it existed is one of these.
TEST(ReplayJsonTest, WholeDocumentAcceptsOneWithNoCanvas)
{
    const auto document = read(nlohmann::json{
        {"magic", "antwika-replay"},
        {"version", 1},
        {"events", nlohmann::json::array()},
    });

    EXPECT_FALSE(document.canvas.has_value());
}

// A document stating no version at all is version 1.
TEST(ReplayJsonTest, WholeDocumentAcceptsOneThatStatesNoVersion)
{
    const auto document = read(nlohmann::json{
        {"magic", "antwika-replay"},
        {"events", nlohmann::json::array({aRecord(0, "a.b")})},
    });

    EXPECT_EQ(document.events.size(), 1U);
}

TEST(ReplayJsonTest, WholeDocumentThrowsWhenTheCanvasIsMalformed)
{
    EXPECT_THROW(
        std::ignore = read(nlohmann::json{
            {"magic", "antwika-replay"},
            {"version", 1},
            {"events", nlohmann::json::array()},
            {"canvas", nlohmann::json{{"width", 1024}}},
        }),
        ReplayFormatError);
}

TEST(ReplayJsonTest, WholeDocumentThrowsWhenAnEventInTheArrayIsMalformed)
{
    EXPECT_THROW(
        std::ignore = read(nlohmann::json{
            {"magic", "antwika-replay"},
            {"version", 1},
            {"events",
             nlohmann::json::array({nlohmann::json{{"tick", 0}}})},
        }),
        ReplayFormatError);
}
