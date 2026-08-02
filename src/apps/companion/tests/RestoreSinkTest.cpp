#include <gtest/gtest.h>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>

#include "antwika/companion/CompanionMemory.hpp"
#include "antwika/companion/Events.hpp"
#include "antwika/companion/Lineage.hpp"
#include "antwika/companion/Pet.hpp"
#include "antwika/companion/RestoreSink.hpp"
#include "antwika/companion/RestoreSource.hpp"
#include "antwika/companion/SaveFormatError.hpp"

using antwika::companion::CompanionMemory;
using antwika::companion::Lineage;
using antwika::companion::LineageMemory;
using antwika::companion::Pet;
using antwika::companion::PetMemory;
using antwika::companion::PetState;
using antwika::companion::restoreEvent;
using antwika::companion::RestoreSink;
using antwika::companion::SaveFormatError;
using antwika::companion::Saying;
using antwika::event::Event;
using antwika::event::TickEvent;

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

    TickEvent restoreAt(
        const antwika::time::Tick tick, const CompanionMemory &memory)
    {
        return TickEvent{.tick = tick, .event = restoreEvent(memory)};
    }

    TEST(RestoreSinkTest, Handle_PutsTheRememberedCompanionInPlace)
    {
        Pet pet;
        Lineage lineage;
        RestoreSink sink(pet, lineage);

        sink.handle(restoreAt(0, lived()));

        EXPECT_EQ(pet.remember(), lived().pet);
        EXPECT_EQ(lineage.remember(), lived().lineage);
    }

    // The numbers a companion is balanced with are this build's.
    // So restoring one must not carry a configuration with it.
    TEST(RestoreSinkTest, Handle_KeepsTheBalanceTheSessionWasGiven)
    {
        antwika::companion::PetConfig brisk;
        brisk.hungerPeriodTicks = 1;

        Pet pet(brisk);
        Lineage lineage;
        RestoreSink sink(pet, lineage);

        sink.handle(restoreAt(0, lived()));

        EXPECT_EQ(pet.settings().hungerPeriodTicks, 1U);
    }

    TEST(RestoreSinkTest, Handle_IgnoresAnythingThatIsNotARestore)
    {
        Pet pet;
        Lineage lineage;
        RestoreSink sink(pet, lineage);

        const auto before = pet.remember();

        sink.handle(TickEvent{
            .tick = 0,
            .event = Event{.name = antwika::engine::events::kTick}});

        EXPECT_EQ(pet.remember(), before);
    }

    // A recording naming an impossible companion is refused.
    // Exactly as a file naming one is.
    TEST(RestoreSinkTest, Handle_RefusesAPayloadThatIsNotACompanion)
    {
        Pet pet;
        Lineage lineage;
        RestoreSink sink(pet, lineage);

        EXPECT_THROW(
            sink.handle(TickEvent{
                .tick = 0,
                .event = Event{
                    .name = antwika::companion::events::kRestore,
                    .payload = "not a document at all"}}),
            SaveFormatError);
    }
} // namespace
