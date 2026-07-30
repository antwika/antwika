#include <gtest/gtest.h>

#include <vector>

#include <antwika/gfx/Size.hpp>

#include "antwika/replay/ReplayDocument.hpp"
#include "antwika/replay/ReplayFormatError.hpp"
#include "antwika/replay/ReplayJson.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::gfx::Size;
using antwika::replay::replayFromJson;
using antwika::replay::replayToJson;
using antwika::replay::ReplayDocument;
using antwika::replay::ReplayFormatError;

TEST(ReplayJsonTest, RoundTripsZeroEvents)
{
    EXPECT_EQ(
        replayFromJson(replayToJson({})).events,
        std::vector<TickEvent>{});
}

TEST(ReplayJsonTest, RoundTripsManyEventsInOrder)
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
    EXPECT_EQ(
        replayFromJson(replayToJson(ReplayDocument{.events = events})),
        (ReplayDocument{.events = events}));
}

TEST(ReplayJsonTest, ReplayToJsonProducesTheExpectedEnvelope)
{
    const auto document = replayToJson({});
    EXPECT_EQ(document.at("magic"), "antwika-replay");
    EXPECT_EQ(document.at("version"), 1);
    EXPECT_TRUE(document.at("events").is_array());
    EXPECT_TRUE(document.at("events").empty());
}

TEST(ReplayJsonTest, ReplayFromJsonThrowsWhenInputIsNotAnObject)
{
    EXPECT_THROW(
        (void)replayFromJson(nlohmann::json::array()), ReplayFormatError);
}

TEST(ReplayJsonTest, ReplayFromJsonThrowsWhenMagicFieldIsMissing)
{
    EXPECT_THROW(
        (void)replayFromJson(nlohmann::json{
            {"version", 1},
            {"events", nlohmann::json::array()},
        }),
        ReplayFormatError);
}

TEST(ReplayJsonTest, ReplayFromJsonThrowsOnBadMagic)
{
    EXPECT_THROW(
        (void)replayFromJson(nlohmann::json{
            {"magic", "nope"},
            {"version", 1},
            {"events", nlohmann::json::array()},
        }),
        ReplayFormatError);
}

TEST(ReplayJsonTest, ReplayFromJsonThrowsOnUnsupportedVersion)
{
    EXPECT_THROW(
        (void)replayFromJson(nlohmann::json{
            {"magic", "antwika-replay"},
            {"version", 2},
            {"events", nlohmann::json::array()},
        }),
        ReplayFormatError);
}

TEST(ReplayJsonTest, ReplayFromJsonThrowsWhenEventsFieldIsNotAnArray)
{
    EXPECT_THROW(
        (void)replayFromJson(nlohmann::json{
            {"magic", "antwika-replay"},
            {"version", 1},
            {"events", "not an array"},
        }),
        ReplayFormatError);
}

TEST(ReplayJsonTest, ReplayToJsonWritesNoCanvasWhenTheDocumentHasNone)
{
    EXPECT_FALSE(replayToJson({}).contains("canvas"));
}

TEST(ReplayJsonTest, RoundTripsTheCanvasTheRecordingWasMadeAgainst)
{
    const ReplayDocument document{
        .events = {},
        .canvas = Size{.width = 1024, .height = 640},
    };

    const auto encoded = replayToJson(document);
    EXPECT_EQ(encoded.at("canvas").at("width"), 1024);
    EXPECT_EQ(encoded.at("canvas").at("height"), 640);
    EXPECT_EQ(replayFromJson(encoded), document);
}

// The whole point of the field being optional.
// Every replay checked in before it existed is one of these.
TEST(ReplayJsonTest, ReplayFromJsonAcceptsADocumentWithNoCanvas)
{
    const auto document = replayFromJson(nlohmann::json{
        {"magic", "antwika-replay"},
        {"version", 1},
        {"events", nlohmann::json::array()},
    });

    EXPECT_FALSE(document.canvas.has_value());
}

TEST(ReplayJsonTest, ReplayFromJsonThrowsWhenTheCanvasIsMalformed)
{
    EXPECT_THROW(
        (void)replayFromJson(nlohmann::json{
            {"magic", "antwika-replay"},
            {"version", 1},
            {"events", nlohmann::json::array()},
            {"canvas", nlohmann::json{{"width", 1024}}},
        }),
        ReplayFormatError);
}

TEST(ReplayJsonTest, ReplayFromJsonThrowsWhenAnEventInTheArrayIsMalformed)
{
    EXPECT_THROW(
        (void)replayFromJson(nlohmann::json{
            {"magic", "antwika-replay"},
            {"version", 1},
            {"events", nlohmann::json::array({nlohmann::json{{"tick", 0}}})},
        }),
        ReplayFormatError);
}
