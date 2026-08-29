#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <limits>

#include "antwika/replay/EventJson.hpp"

using antwika::event::Event;
using antwika::event::EventName;
using antwika::event::TickEvent;

TEST(EventJsonTest, ToJson_ProducesTheExpectedShape)
{
    TickEvent event{
        .tick = 42,
        .event = Event{.name = EventName{"game.score_increment"}, .payload = "5"},
    };

    EXPECT_EQ(
        nlohmann::json(event),
        (nlohmann::json{
            {"tick", 42},
            {"event", {{"name", "game.score_increment"}, {"payload", "5"}}},
        }));
}

TEST(EventJsonTest, FromJson_RoundTripsATypicalEvent)
{
    TickEvent event{
        .tick = 42,
        .event = Event{
            .name = EventName{"game.score_increment"},
            .payload = R"({"amount":5})",
        },
    };
    EXPECT_EQ(nlohmann::json(event).get<TickEvent>(), event);
}

TEST(EventJsonTest, FromJson_RoundTripsAnEmptyNameAndPayload)
{
    TickEvent event{.tick = 0, .event = Event{}};
    EXPECT_EQ(nlohmann::json(event).get<TickEvent>(), event);
}

TEST(EventJsonTest, FromJson_RoundTripsALargeTick)
{
    TickEvent event{
        .tick = static_cast<antwika::time::Tick>(
            std::numeric_limits<std::int64_t>::max()),
        .event = Event{.name = EventName{"max-tick"}},
    };
    EXPECT_EQ(nlohmann::json(event).get<TickEvent>(), event);
}

TEST(EventJsonTest, FromJson_ThrowsOnAMissingTick)
{
    const nlohmann::json j{
        {"event", {{"name", "a"}, {"payload", ""}}},
    };
    EXPECT_THROW((void)j.get<TickEvent>(), nlohmann::json::out_of_range);
}

TEST(EventJsonTest, FromJson_ThrowsOnAMissingName)
{
    const nlohmann::json j{
        {"tick", 0},
        {"event", {{"payload", ""}}},
    };
    EXPECT_THROW((void)j.get<TickEvent>(), nlohmann::json::out_of_range);
}

TEST(EventJsonTest, FromJson_ThrowsOnANonStringPayload)
{
    const nlohmann::json j{
        {"tick", 0},
        {"event", {{"name", "a"}, {"payload", 5}}},
    };
    EXPECT_THROW((void)j.get<TickEvent>(), nlohmann::json::type_error);
}
