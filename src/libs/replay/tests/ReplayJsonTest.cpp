#include <gtest/gtest.h>

#include "antwika/replay/ReplayFormatError.hpp"
#include "antwika/replay/ReplayJson.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::replay::replayFromJson;
using antwika::replay::replayToJson;
using antwika::replay::ReplayFormatError;

TEST(ReplayJsonTest, RoundTripsZeroEvents)
{
    EXPECT_EQ(
        replayFromJson(replayToJson({})), std::vector<TickEvent>{});
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
    EXPECT_EQ(replayFromJson(replayToJson(events)), events);
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
