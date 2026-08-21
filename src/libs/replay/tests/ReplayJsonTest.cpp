#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <limits>
#include <string>
#include <tuple>
#include <vector>

#include <antwika/geometry/Size.hpp>

#include "antwika/replay/EventJson.hpp"
#include "antwika/replay/ReplayDocument.hpp"
#include "antwika/replay/ReplayFormatError.hpp"
#include "antwika/replay/ReplayHeader.hpp"
#include "antwika/replay/ReplayJson.hpp"
#include "antwika/replay/ReplayMigrations.hpp"
#include "antwika/replay/ReplayVersions.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::geometry::Size;
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
}

TEST(ReplayJsonTest, ReadHeader_RoundTripsItsOwnEncoding)
{
    const ReplayHeader header{
        .version = kReplayDocumentVersion,
        .canvasSize = Size{.width = 1024, .height = 640}};

    EXPECT_EQ(readHeader(replayHeaderToJson(header)), header);
}

TEST(ReplayJsonTest, ReplayHeaderToJson_StatesFormatAndVersion)
{
    const auto headerJson = replayHeaderToJson(ReplayHeader{});

    EXPECT_EQ(headerJson.at("magic"), "antwika-replay");
    EXPECT_EQ(headerJson.at("version"), kReplayDocumentVersion);
}

TEST(ReplayJsonTest, ReplayHeaderToJson_WritesNoCanvasWhenNone)
{
    EXPECT_FALSE(replayHeaderToJson(ReplayHeader{}).contains("canvas"));
}

TEST(ReplayJsonTest, ReplayHeaderToJson_DumpsToOneLine)
{
    const auto text = replayHeaderToJson(ReplayHeader{}).dump();

    EXPECT_EQ(text.find('\n'), std::string::npos) << text;
}

TEST(ReplayJsonTest, ReadHeader_ThrowsOnBadMagic)
{
    EXPECT_THROW(
        std::ignore = readHeader(nlohmann::json{{"magic", "nope"}}),
        ReplayFormatError);
}

TEST(ReplayJsonTest, ReadHeader_ThrowsOnMissingMagic)
{
    EXPECT_THROW(
        std::ignore = readHeader(nlohmann::json{{"version", 2}}),
        ReplayFormatError);
}

TEST(ReplayJsonTest, ReadHeader_ThrowsOnAMalformedCanvas)
{
    EXPECT_THROW(
        std::ignore = readHeader(nlohmann::json{
            {"magic", "antwika-replay"},
            {"canvas", nlohmann::json{{"width", 1024}}}}),
        ReplayFormatError);
}

TEST(ReplayJsonTest, ReadHeader_ThrowsOnAnExtentPastUint32)
{
    EXPECT_THROW(
        std::ignore = readHeader(nlohmann::json{
            {"magic", "antwika-replay"},
            {"canvas",
             nlohmann::json{
                 {"width", std::int64_t{4294967297}}, {"height", 640}}}}),
        ReplayFormatError);
}

TEST(ReplayJsonTest, ReadHeader_ReadsAnExtentAtTheMaximum)
{
    const ReplayHeader header = readHeader(nlohmann::json{
        {"magic", "antwika-replay"},
        {"canvas",
         nlohmann::json{
             {"width", std::numeric_limits<std::uint32_t>::max()},
             {"height", 640}}}});

    ASSERT_TRUE(header.canvasSize.has_value());
    EXPECT_EQ(
        header.canvasSize->width, std::numeric_limits<std::uint32_t>::max());
}

TEST(ReplayJsonTest, ReadHeader_ThrowsOnAnUnknownCanvasMember)
{
    EXPECT_THROW(
        std::ignore = readHeader(nlohmann::json{
            {"magic", "antwika-replay"},
            {"canvas",
             nlohmann::json{
                 {"width", 1024}, {"height", 640}, {"depth", 3}}}}),
        ReplayFormatError);
}

TEST(ReplayJsonTest, ReadHeader_PassesOverAnUnknownMember)
{
    const auto header = readHeader(
        nlohmann::json{{"magic", "antwika-replay"}, {"novel", 4}});

    EXPECT_EQ(header.version, 1U);
}

TEST(ReplayJsonTest, ReadRecords_RoundTripInOrder)
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

TEST(ReplayJsonTest, ReadRecords_ThrowOnANonSequence)
{
    EXPECT_THROW(
        std::ignore = readRecords(nlohmann::json::object()),
        ReplayFormatError);
}

TEST(ReplayJsonTest, ReadRecords_ThrowOnAMalformedRecord)
{
    EXPECT_THROW(
        std::ignore = readRecords(
            nlohmann::json::array({nlohmann::json{{"tick", 0}}})),
        ReplayFormatError);
}

TEST(ReplayJsonTest, ReadRecords_ThrowOnAnUnknownRecordMember)
{
    auto record = aRecord(0, "a.b");
    record["colour"] = "blue";

    EXPECT_THROW(
        std::ignore = readRecords(nlohmann::json::array({record})),
        ReplayFormatError);
}

TEST(ReplayJsonTest, ReadRecords_ThrowOnAnUnknownEventMember)
{
    auto record = aRecord(0, "a.b");
    record["event"]["colour"] = "blue";

    EXPECT_THROW(
        std::ignore = readRecords(nlohmann::json::array({record})),
        ReplayFormatError);
}

TEST(ReplayJsonTest, ReadRecords_ThrowOnANonObjectRecord)
{
    EXPECT_THROW(
        std::ignore = readRecords(nlohmann::json::array({42})),
        ReplayFormatError);
}

TEST(ReplayJsonTest, ReadRecords_ThrowWhenATickGoesBackwards)
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

TEST(ReplayJsonTest, ReadRecords_AcceptSeveralOnOneTick)
{
    const auto records = nlohmann::json::array(
        {aRecord(4, "a.b"), aRecord(4, "c.d"), aRecord(5, "e.f")});

    EXPECT_EQ(readRecords(records).size(), 3U);
}

TEST(ReplayJsonTest, ReadRecords_ThrowOnASecondHeader)
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

TEST(ReplayJsonTest, Read_TakesAWholeDocumentAsHeaderAndRecords)
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

TEST(ReplayJsonTest, Read_KeepsTheRecordedCanvas)
{
    const auto document = read(nlohmann::json{
        {"magic", "antwika-replay"},
        {"version", 1},
        {"events", nlohmann::json::array()},
        {"canvas", nlohmann::json{{"width", 1024}, {"height", 640}}},
    });

    EXPECT_EQ(document.canvasSize, (Size{.width = 1024, .height = 640}));
}

TEST(ReplayJsonTest, Read_ThrowsOnANonObjectDocument)
{
    EXPECT_THROW(
        std::ignore = read(nlohmann::json::array()), ReplayFormatError);
}

TEST(ReplayJsonTest, Read_ThrowsOnAMissingMagicField)
{
    EXPECT_THROW(
        std::ignore = read(nlohmann::json{
            {"version", 1},
            {"events", nlohmann::json::array()},
        }),
        ReplayFormatError);
}

TEST(ReplayJsonTest, Read_ThrowsOnANonArrayEventsField)
{
    EXPECT_THROW(
        std::ignore = read(nlohmann::json{
            {"magic", "antwika-replay"},
            {"version", 1},
            {"events", "not an array"},
        }),
        ReplayFormatError);
}

TEST(ReplayJsonTest, Read_AcceptsADocumentWithNoCanvas)
{
    const auto document = read(nlohmann::json{
        {"magic", "antwika-replay"},
        {"version", 1},
        {"events", nlohmann::json::array()},
    });

    EXPECT_FALSE(document.canvasSize.has_value());
}

TEST(ReplayJsonTest, Read_AcceptsADocumentStatingNoVersion)
{
    const auto document = read(nlohmann::json{
        {"magic", "antwika-replay"},
        {"events", nlohmann::json::array({aRecord(0, "a.b")})},
    });

    EXPECT_EQ(document.events.size(), 1U);
}

TEST(ReplayJsonTest, Read_ThrowsOnAMalformedCanvas)
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

TEST(ReplayJsonTest, Read_ThrowsOnAMalformedEvent)
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
