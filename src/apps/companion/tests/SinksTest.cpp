#include <chrono>
#include <cstdint>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockWindow.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/time/fakes/FakeSleeper.hpp>

#include "antwika/companion/PacingSink.hpp"
#include "antwika/companion/Pet.hpp"
#include "antwika/companion/PetScene.hpp"
#include "antwika/companion/PetSink.hpp"
#include "antwika/companion/RenderSink.hpp"
#include "antwika/companion/TapSink.hpp"

using antwika::companion::PacingSink;
using antwika::companion::Pet;
using antwika::companion::PetConfig;
using antwika::companion::PetScene;
using antwika::companion::PetSink;
using antwika::companion::PetState;
using antwika::companion::RenderSink;
using antwika::companion::TapSink;
using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockWindow;
using antwika::gfx::Size;
using antwika::input::InputEventCodec;
using antwika::input::KeyPressed;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::input::PointerButtonReleased;
using antwika::log::mocks::MockLogger;
using antwika::time::fakes::FakeSleeper;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace
{
    constexpr Size kCanvas{.width = 256, .height = 256};

    constexpr PetConfig kQuick{
        .dayTicks = 4,
        .nightTicks = 6,
        .hungerPeriodTicks = 1,
        .starvePeriodTicks = 1000,
        .restPeriodTicks = 1000,
        .hungerMax = 4,
        .hungerThreshold = 2,
        .feedRelief = 2,
        .feedJoy = 1,
        .disturbCost = 2,
        .happinessMax = 6,
        .happinessStart = 4};

    TickEvent tick()
    {
        return TickEvent{
            .tick = 0,
            .event = Event{.name = antwika::engine::events::kTick}};
    }

    TickEvent other()
    {
        return TickEvent{
            .tick = 0, .event = Event{.name = "something.else"}};
    }

    TickEvent press(
        const InputEventCodec &codec,
        const MouseButton button = MouseButton::Left)
    {
        return TickEvent{
            .tick = 0,
            .event = codec.encode(PointerButtonPressed{
                .button = button, .position = {.x = 64, .y = 64}})};
    }

    TEST(PetSinkTest, OnlyATickStepsTheCompanion)
    {
        Pet pet(kQuick);
        PetSink sink(pet);

        sink.handle(other());
        EXPECT_EQ(pet.ticks(), 0U);

        sink.handle(tick());
        EXPECT_EQ(pet.ticks(), 1U);
    }

    TEST(TapSinkTest, ALeftPressAnywhereFeedsAHungryCompanion)
    {
        Pet pet(kQuick);
        const InputEventCodec codec;
        TapSink sink(pet, codec);

        pet.step();
        pet.step();
        ASSERT_TRUE(pet.hungry());

        sink.handle(press(codec));

        EXPECT_EQ(pet.meals(), 1U);
        EXPECT_EQ(pet.hunger(), 0U);
    }

    TEST(TapSinkTest, OnlyALeftPressIsATap)
    {
        Pet pet(kQuick);
        const InputEventCodec codec;
        TapSink sink(pet, codec);

        pet.step();
        pet.step();

        sink.handle(press(codec, MouseButton::Right));
        sink.handle(TickEvent{
            .tick = 0,
            .event = codec.encode(PointerButtonReleased{
                .button = MouseButton::Left,
                .position = {.x = 64, .y = 64}})});
        sink.handle(TickEvent{
            .tick = 0,
            .event = codec.encode(KeyPressed{
                .key = antwika::input::Key::Space, .modifiers = {}})});

        EXPECT_EQ(pet.meals(), 0U);
    }

    TEST(TapSinkTest, AnEventThatIsNotInputAtAllIsIgnored)
    {
        Pet pet(kQuick);
        const InputEventCodec codec;
        TapSink sink(pet, codec);

        sink.handle(tick());
        sink.handle(other());

        EXPECT_EQ(pet.ticks(), 0U);
        EXPECT_EQ(pet.meals(), 0U);
    }

    TEST(RenderSinkTest, ATickDrawsAFrame)
    {
        const Pet pet(kQuick);
        const PetScene scene;
        NiceMock<MockRenderer> renderer;
        NiceMock<MockWindow> window;
        ON_CALL(window, isOpen()).WillByDefault(Return(true));
        ON_CALL(window, renderer()).WillByDefault(ReturnRef(renderer));

        EXPECT_CALL(renderer, present()).Times(1);

        RenderSink sink(window, scene, pet, kCanvas);
        sink.handle(tick());
    }

    TEST(RenderSinkTest, AClosedWindowAndANonTickDrawNothing)
    {
        const Pet pet(kQuick);
        const PetScene scene;
        NiceMock<MockWindow> window;
        ON_CALL(window, isOpen()).WillByDefault(Return(false));

        EXPECT_CALL(window, renderer()).Times(0);

        RenderSink sink(window, scene, pet, kCanvas);
        sink.handle(other());
        sink.handle(tick());
    }

    TEST(PacingSinkTest, ATickWaitsOutItsIntervalAndNothingElseDoes)
    {
        NiceMock<MockLogger> logger;
        FakeSleeper sleeper;
        PacingSink sink(logger, sleeper, std::chrono::milliseconds{50});

        sink.handle(other());
        EXPECT_TRUE(sleeper.requested().empty());

        sink.handle(tick());
        ASSERT_EQ(sleeper.requested().size(), 1U);
        EXPECT_EQ(sleeper.requested()[0], std::chrono::milliseconds{50});
    }
} // namespace
