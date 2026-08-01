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

#include "antwika/companion/Pet.hpp"
#include "antwika/companion/PetLayout.hpp"
#include "antwika/companion/ReviveSink.hpp"
#include "antwika/companion/TapSink.hpp"

using antwika::companion::Pet;
using antwika::companion::PetConfig;
using antwika::companion::PetState;
using antwika::companion::reviveButtonRect;
using antwika::companion::ReviveSink;
using antwika::companion::TapSink;
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

    // A day of four ticks and a night of six, and one tap ends it.
    // So a test reaches a perished companion in two calls.
    constexpr PetConfig kFragile{
        .dayTicks = 4,
        .nightTicks = 6,
        .hungerPeriodTicks = 1,
        .starvePeriodTicks = 1000,
        .restPeriodTicks = 1000,
        .hungerMax = 8,
        .hungerThreshold = 6,
        .feedRelief = 2,
        .feedJoy = 1,
        .disturbCost = 2,
        .pesterCost = 4,
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

    // One pester at full hunger threshold costs four of four.
    Pet perished()
    {
        Pet pet(kFragile);
        pet.tap();

        return pet;
    }

    TEST(ReviveSinkTest, Handle_APressOnTheButtonStartsANewCompanion)
    {
        const InputEventCodec codec;
        Pet pet = perished();
        ASSERT_EQ(pet.state(), PetState::Perished);

        ReviveSink sink(pet, codec, kCanvas);
        sink.handle(pressAt(codec, onTheButton()));

        EXPECT_EQ(pet.state(), PetState::Awake);
        EXPECT_EQ(pet.happiness(), kFragile.happinessStart);
        EXPECT_EQ(pet.ticks(), 0);
        EXPECT_EQ(pet.pesters(), 0);
    }

    TEST(ReviveSinkTest, Handle_APressElsewhereLeavesTheCompanionGone)
    {
        const InputEventCodec codec;
        Pet pet = perished();

        ReviveSink sink(pet, codec, kCanvas);
        sink.handle(pressAt(codec, Point{.x = 250, .y = 250}));

        EXPECT_EQ(pet.state(), PetState::Perished);
    }

    // The button is only there while there is nothing else to do.
    // A press on that part of a living companion's window is a tap.
    TEST(ReviveSinkTest, Handle_DoesNothingWhileTheCompanionIsAlive)
    {
        const InputEventCodec codec;
        Pet pet(kFragile);

        ReviveSink sink(pet, codec, kCanvas);
        sink.handle(pressAt(codec, onTheButton()));

        EXPECT_EQ(pet.state(), PetState::Awake);
        EXPECT_EQ(pet.happiness(), kFragile.happinessStart);
    }

    TEST(ReviveSinkTest, Handle_IgnoresAPressOfAnotherButton)
    {
        const InputEventCodec codec;
        Pet pet = perished();

        ReviveSink sink(pet, codec, kCanvas);
        sink.handle(
            pressAt(codec, onTheButton(), MouseButton::Right));

        EXPECT_EQ(pet.state(), PetState::Perished);
    }

    TEST(ReviveSinkTest, Handle_IgnoresAReleaseOnTheButton)
    {
        const InputEventCodec codec;
        Pet pet = perished();
        const Point at = onTheButton();

        ReviveSink sink(pet, codec, kCanvas);
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

        ReviveSink sink(pet, codec, kCanvas);
        sink.handle(TickEvent{
            .tick = 0,
            .event = Event{.name = antwika::engine::events::kTick}});

        EXPECT_EQ(pet.state(), PetState::Perished);
    }

    // The registration order keeps one press meaning one thing.
    // The tap is answered first, by a companion that ignores it.
    // Only then does the button see the same press.
    TEST(ReviveSinkTest, APressIsATapOrAButtonAndNeverBoth)
    {
        const InputEventCodec codec;
        Pet pet = perished();

        TapSink tapSink(pet, codec);
        ReviveSink reviveSink(pet, codec, kCanvas);

        const TickEvent event = pressAt(codec, onTheButton());
        tapSink.handle(event);
        reviveSink.handle(event);

        EXPECT_EQ(pet.state(), PetState::Awake);
        EXPECT_EQ(pet.meals(), 0);
        EXPECT_EQ(pet.pesters(), 0);
        EXPECT_EQ(pet.happiness(), kFragile.happinessStart);
    }
} // namespace
