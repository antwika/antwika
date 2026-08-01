#include <cstdint>

#include <gtest/gtest.h>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/MouseButton.hpp>

#include "antwika/companion/Lineage.hpp"
#include "antwika/companion/Pet.hpp"
#include "antwika/companion/PetLayout.hpp"
#include "antwika/companion/PropSink.hpp"
#include "antwika/companion/ReviveSink.hpp"

using antwika::companion::Lineage;
using antwika::companion::Pet;
using antwika::companion::PetConfig;
using antwika::companion::PetState;
using antwika::companion::PropSink;
using antwika::companion::reviveButtonRect;
using antwika::companion::ReviveSink;
using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::gfx::Point;
using antwika::gfx::Size;
using antwika::input::InputEventCodec;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::input::PointerButtonReleased;

namespace
{
    constexpr Size kCanvas{.width = 256, .height = 256};

    // One game spends everything the companion has.
    // The collapse that follows takes the whole of its ceiling.
    // So a test reaches a grave in a single call.
    constexpr PetConfig kFragile{
        .hungerPeriodTicks = 1000,
        .starvePeriodTicks = 1000,
        .funDecayPeriodTicks = 1000,
        .fretPeriodTicks = 1000,
        .recoverPeriodTicks = 1000,
        .restPeriodTicks = 1000,
        .drainHappyTicks = 1000,
        .drainContentTicks = 1000,
        .drainLowTicks = 1000,
        .drainMiserableTicks = 1000,
        .hungerMax = 4,
        .hungerThreshold = 2,
        .funMax = 4,
        .funStart = 4,
        .playEnergy = 5,
        .energyBase = 5,
        .collapsePenalty = 5,
        .happinessMax = 6,
        .happinessStart = 4};

    Point onTheButton()
    {
        const auto button = reviveButtonRect(kCanvas);

        return Point{
            .x = button->origin.x
                 + static_cast<std::int32_t>(button->size.width) / 2,
            .y = button->origin.y
                 + static_cast<std::int32_t>(button->size.height) / 2};
    }

    TickEvent pressAt(
        const InputEventCodec &codec,
        const Point at,
        const MouseButton button = MouseButton::Left)
    {
        return TickEvent{
            .tick = 0,
            .event = codec.encode(PointerButtonPressed{
                .button = button,
                .position = {.x = at.x, .y = at.y}})};
    }

    Pet perished()
    {
        Pet pet(kFragile);
        pet.play();

        return pet;
    }

    TEST(ReviveSinkTest, Handle_APressOnTheButtonStartsANewCompanion)
    {
        const InputEventCodec codec;
        Pet pet = perished();
        Lineage lineage;
        ASSERT_EQ(pet.state(), PetState::Perished);

        ReviveSink sink(pet, lineage, codec, kCanvas);
        sink.handle(pressAt(codec, onTheButton()));

        EXPECT_EQ(pet.state(), PetState::Awake);
        EXPECT_EQ(pet.energy(), kFragile.energyBase);
        EXPECT_EQ(pet.ticks(), 0U);
        EXPECT_EQ(pet.collapses(), 0U);
    }

    // The record is what a revival leaves behind.
    // It is written here because revive() replaces the companion.
    TEST(ReviveSinkTest, Handle_ARevivalWritesTheRecordAndMovesOn)
    {
        const InputEventCodec codec;
        Pet pet(kFragile);
        Lineage lineage;

        for (int step = 0; step < 7; ++step)
        {
            pet.step();
        }
        pet.play();
        ASSERT_EQ(pet.state(), PetState::Perished);
        const auto lived = pet.ticks();

        ReviveSink sink(pet, lineage, codec, kCanvas);
        sink.handle(pressAt(codec, onTheButton()));

        EXPECT_EQ(lineage.generation(), 2U);
        EXPECT_EQ(lineage.bestTicks(), lived);
    }

    TEST(ReviveSinkTest, Handle_APressElsewhereLeavesTheCompanionGone)
    {
        const InputEventCodec codec;
        Pet pet = perished();
        Lineage lineage;

        ReviveSink sink(pet, lineage, codec, kCanvas);
        sink.handle(pressAt(codec, Point{.x = 250, .y = 250}));

        EXPECT_EQ(pet.state(), PetState::Perished);
        EXPECT_EQ(lineage.generation(), 1U);
    }

    // The button is only there while there is nothing else to do.
    // A press on that part of a living companion's window is a prod.
    TEST(ReviveSinkTest, Handle_DoesNothingWhileTheCompanionIsAlive)
    {
        const InputEventCodec codec;
        Pet pet(kFragile);
        Lineage lineage;

        ReviveSink sink(pet, lineage, codec, kCanvas);
        sink.handle(pressAt(codec, onTheButton()));

        EXPECT_EQ(pet.state(), PetState::Awake);
        EXPECT_EQ(pet.happiness(), kFragile.happinessStart);
    }

    TEST(ReviveSinkTest, Handle_IgnoresAPressOfAnotherButton)
    {
        const InputEventCodec codec;
        Pet pet = perished();
        Lineage lineage;

        ReviveSink sink(pet, lineage, codec, kCanvas);
        sink.handle(pressAt(codec, onTheButton(), MouseButton::Right));

        EXPECT_EQ(pet.state(), PetState::Perished);
    }

    TEST(ReviveSinkTest, Handle_IgnoresAReleaseOnTheButton)
    {
        const InputEventCodec codec;
        Pet pet = perished();
        Lineage lineage;
        const Point at = onTheButton();

        ReviveSink sink(pet, lineage, codec, kCanvas);
        sink.handle(TickEvent{
            .tick = 0,
            .event = codec.encode(PointerButtonReleased{
                .button = MouseButton::Left,
                .position = {.x = at.x, .y = at.y}})});

        EXPECT_EQ(pet.state(), PetState::Perished);
    }

    TEST(ReviveSinkTest, Handle_IgnoresAnEventThatIsNotInputAtAll)
    {
        const InputEventCodec codec;
        Pet pet = perished();
        Lineage lineage;

        ReviveSink sink(pet, lineage, codec, kCanvas);
        sink.handle(TickEvent{
            .tick = 0,
            .event = Event{.name = antwika::engine::events::kTick}});

        EXPECT_EQ(pet.state(), PetState::Perished);
    }

    // The registration order keeps one press meaning one thing.
    // The props are answered first, by a companion that ignores them.
    // Only then does the button see the same press.
    TEST(ReviveSinkTest, APressIsAPropOrAButtonAndNeverBoth)
    {
        const InputEventCodec codec;
        Pet pet = perished();
        Lineage lineage;

        PropSink propSink(pet, codec, kCanvas);
        ReviveSink reviveSink(pet, lineage, codec, kCanvas);

        const TickEvent event = pressAt(codec, onTheButton());
        propSink.handle(event);
        reviveSink.handle(event);

        EXPECT_EQ(pet.state(), PetState::Awake);
        EXPECT_EQ(pet.meals(), 0U);
        EXPECT_EQ(pet.plays(), 0U);
        EXPECT_EQ(pet.pesters(), 0U);
        EXPECT_EQ(pet.happiness(), kFragile.happinessStart);
    }
} // namespace
