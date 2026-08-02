#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/replay/ReplaySource.hpp>

#include "antwika/companion/CompanionMemory.hpp"
#include "antwika/companion/Events.hpp"
#include "antwika/companion/Pet.hpp"
#include "antwika/companion/PetSave.hpp"
#include "antwika/companion/RestoreSource.hpp"

using antwika::companion::CompanionMemory;
using antwika::companion::companionMemoryFromJson;
using antwika::companion::LineageMemory;
using antwika::companion::PetMemory;
using antwika::companion::PetState;
using antwika::companion::restoreEvent;
using antwika::companion::RestoreSource;
using antwika::companion::Saying;
using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::replay::ReplaySource;

namespace
{
    CompanionMemory lived()
    {
        return CompanionMemory{
            .pet =
                PetMemory{
                    .ticks = 100,
                    .state = PetState::Awake,
                    .saying = Saying::Hello,
                    .sayingTicksLeft = 3,
                    .hunger = 5,
                    .fun = 4,
                    .happiness = 6,
                    .energy = 14,
                    .day = 3,
                    .meals = 7,
                    .plays = 5,
                    .disturbances = 2,
                    .pesters = 1,
                    .collapses = 0,
                    .woken = false},
            .lineage = LineageMemory{.generation = 2, .bestTicks = 50}};
    }

    std::vector<TickEvent> onePress()
    {
        return {TickEvent{
            .tick = 0,
            .event = Event{.name = "companion.not_a_real_event"}}};
    }

    TEST(RestoreSourceTest, RestoreEvent_CarriesTheWholeSavedDocument)
    {
        const Event announcement = restoreEvent(lived());

        EXPECT_EQ(
            announcement.name, antwika::companion::events::kRestore);

        // The file's own format, so one codec reads both.
        EXPECT_EQ(
            companionMemoryFromJson(
                nlohmann::json::parse(announcement.payload)),
            lived());
    }

    TEST(RestoreSourceTest, EventsFor_AnnouncesTheCompanionFirstOfAll)
    {
        ReplaySource inner(onePress());
        RestoreSource source(inner, lived());

        const auto events = source.eventsFor(0);

        ASSERT_EQ(events.size(), 2U);
        EXPECT_EQ(events[0].name, antwika::companion::events::kRestore);
        EXPECT_EQ(events[1].name, "companion.not_a_real_event");
    }

    // Once, and never again.
    // A second one would replace the companion halfway through.
    TEST(RestoreSourceTest, EventsFor_AnnouncesItOnceAndNoMore)
    {
        ReplaySource inner({});
        RestoreSource source(inner, lived());

        EXPECT_EQ(source.eventsFor(0).size(), 1U);
        EXPECT_TRUE(source.eventsFor(1).empty());
        EXPECT_TRUE(source.eventsFor(2).empty());
    }

    // Which is every replay, and every live run with no store.
    TEST(RestoreSourceTest, EventsFor_AddsNothingWithNoCompanionToAdd)
    {
        ReplaySource inner(onePress());
        RestoreSource source(inner, std::nullopt);

        const auto events = source.eventsFor(0);

        ASSERT_EQ(events.size(), 1U);
        EXPECT_EQ(events[0].name, "companion.not_a_real_event");
    }
} // namespace
