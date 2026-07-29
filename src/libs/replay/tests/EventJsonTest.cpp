#include <gtest/gtest.h>

#include <limits>

#include "antwika/replay/EventJson.hpp"
#include "antwika/replay/ReplayFormatError.hpp"

using antwika::event::Event;
using antwika::event::TimedEvent;
using antwika::replay::eventFromJson;
using antwika::replay::eventToJson;
using antwika::replay::ReplayFormatError;

TEST(EventJsonTest, EventToJsonProducesTheExpectedShape)
{
    TimedEvent event{
        .tick = 42,
        .event = Event{.name = "game.score_increment", .payload = "5"},
    };

    EXPECT_EQ(
        eventToJson(event),
        (nlohmann::json{
            {"tick", 42},
            {"event", {{"name", "game.score_increment"}, {"payload", "5"}}},
        }));
}

TEST(EventJsonTest, EventFromJsonRoundTripsATypicalEvent)
{
    TimedEvent event{
        .tick = 42,
        .event = Event{
            .name = "game.score_increment",
            .payload = R"({"amount":5})",
        },
    };
    EXPECT_EQ(eventFromJson(eventToJson(event)), event);
}

TEST(EventJsonTest, EventFromJsonRoundTripsEmptyNameAndPayload)
{
    TimedEvent event{.tick = 0, .event = Event{}};
    EXPECT_EQ(eventFromJson(eventToJson(event)), event);
}

// Not numeric_limits<Tick>::max().
// The schema library mishandles "minimum" above INT64_MAX.
// So the JSON tick range tops out there, unlike BinaryEventCodec's.
TEST(EventJsonTest, EventFromJsonRoundTripsLargeTickValue)
{
    TimedEvent event{
        .tick = static_cast<antwika::time::Tick>(
            std::numeric_limits<std::int64_t>::max()),
        .event = Event{.name = "max-tick"},
    };
    EXPECT_EQ(eventFromJson(eventToJson(event)), event);
}

TEST(EventJsonTest, EventFromJsonThrowsWhenInputIsNotAnObject)
{
    EXPECT_THROW(
        (void)eventFromJson(nlohmann::json::array({1, 2, 3})),
        ReplayFormatError);
}

TEST(EventJsonTest, EventFromJsonThrowsWhenTickFieldIsMissing)
{
    EXPECT_THROW(
        (void)eventFromJson(nlohmann::json{
            {"event", {{"name", "a"}, {"payload", ""}}},
        }),
        ReplayFormatError);
}

TEST(EventJsonTest, EventFromJsonThrowsWhenTickFieldIsNegative)
{
    EXPECT_THROW(
        (void)eventFromJson(nlohmann::json{
            {"tick", -1},
            {"event", {{"name", "a"}, {"payload", ""}}},
        }),
        ReplayFormatError);
}

TEST(EventJsonTest, EventFromJsonThrowsWhenEventFieldIsMissing)
{
    EXPECT_THROW(
        (void)eventFromJson(nlohmann::json{{"tick", 0}}), ReplayFormatError);
}

TEST(EventJsonTest, EventFromJsonThrowsWhenNameFieldIsMissing)
{
    EXPECT_THROW(
        (void)eventFromJson(nlohmann::json{
            {"tick", 0},
            {"event", {{"payload", ""}}},
        }),
        ReplayFormatError);
}

TEST(EventJsonTest, EventFromJsonThrowsWhenPayloadFieldIsNotAString)
{
    EXPECT_THROW(
        (void)eventFromJson(nlohmann::json{
            {"tick", 0},
            {"event", {{"name", "a"}, {"payload", 5}}},
        }),
        ReplayFormatError);
}

TEST(EventJsonTest, EventFromJsonThrowsOnAnUnknownTopLevelField)
{
    EXPECT_THROW(
        (void)eventFromJson(nlohmann::json{
            {"tick", 0},
            {"event", {{"name", "a"}, {"payload", ""}}},
            {"unexpected", true},
        }),
        ReplayFormatError);
}
