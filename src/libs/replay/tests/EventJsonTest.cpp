#include <gtest/gtest.h>

#include <limits>

#include "antwika/replay/EventJson.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;

TEST(EventJsonTest, ToJsonProducesTheExpectedShape)
{
    TickEvent event{
        .tick = 42,
        .event = Event{.name = "game.score_increment", .payload = "5"},
    };

    EXPECT_EQ(
        nlohmann::json(event),
        (nlohmann::json{
            {"tick", 42},
            {"event", {{"name", "game.score_increment"}, {"payload", "5"}}},
        }));
}

TEST(EventJsonTest, FromJsonRoundTripsATypicalEvent)
{
    TickEvent event{
        .tick = 42,
        .event = Event{
            .name = "game.score_increment",
            .payload = R"({"amount":5})",
        },
    };
    EXPECT_EQ(nlohmann::json(event).get<TickEvent>(), event);
}

TEST(EventJsonTest, FromJsonRoundTripsEmptyNameAndPayload)
{
    TickEvent event{.tick = 0, .event = Event{}};
    EXPECT_EQ(nlohmann::json(event).get<TickEvent>(), event);
}

// Not numeric_limits<Tick>::max().
// The schema library mishandles "minimum" above INT64_MAX.
// So the JSON tick range tops out there, short of Tick's full width.
TEST(EventJsonTest, FromJsonRoundTripsLargeTickValue)
{
    TickEvent event{
        .tick = static_cast<antwika::time::Tick>(
            std::numeric_limits<std::int64_t>::max()),
        .event = Event{.name = "max-tick"},
    };
    EXPECT_EQ(nlohmann::json(event).get<TickEvent>(), event);
}

// These overloads validate nothing.
// That is the schema's job, one level up.
// A whole document is checked there, before any of it is decoded.
// What they do promise is that a missing field throws.
// Rather than decoding to a silently defaulted value.
TEST(EventJsonTest, FromJsonThrowsWhenTickFieldIsMissing)
{
    const nlohmann::json j{
        {"event", {{"name", "a"}, {"payload", ""}}},
    };
    EXPECT_THROW((void)j.get<TickEvent>(), nlohmann::json::out_of_range);
}

TEST(EventJsonTest, FromJsonThrowsWhenNameFieldIsMissing)
{
    const nlohmann::json j{
        {"tick", 0},
        {"event", {{"payload", ""}}},
    };
    EXPECT_THROW((void)j.get<TickEvent>(), nlohmann::json::out_of_range);
}

TEST(EventJsonTest, FromJsonThrowsWhenPayloadFieldIsNotAString)
{
    const nlohmann::json j{
        {"tick", 0},
        {"event", {{"name", "a"}, {"payload", 5}}},
    };
    EXPECT_THROW((void)j.get<TickEvent>(), nlohmann::json::type_error);
}
