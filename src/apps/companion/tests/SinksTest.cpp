#include <chrono>
#include <cstdint>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockWindow.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/i18n/Translator.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/time/fakes/FakeSleeper.hpp>

#include "antwika/companion/Lineage.hpp"
#include "antwika/companion/PacingSink.hpp"
#include "antwika/companion/Pet.hpp"
#include "antwika/companion/PetLayout.hpp"
#include "antwika/companion/PetScene.hpp"
#include "antwika/companion/PetSink.hpp"
#include "antwika/companion/PropSink.hpp"
#include "antwika/companion/RenderSink.hpp"

using antwika::companion::layoutFor;
using antwika::companion::Lineage;
using antwika::companion::PacingSink;
using antwika::companion::Pet;
using antwika::companion::PetConfig;
using antwika::companion::PetScene;
using antwika::companion::PetSink;
using antwika::companion::PetState;
using antwika::companion::Prop;
using antwika::companion::propAt;
using antwika::companion::propBox;
using antwika::companion::PropSink;
using antwika::companion::RenderSink;
using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockWindow;
using antwika::gfx::Point;
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
        .hungerPeriodTicks = 1,
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
        .feedRelief = 2,
        .funMax = 4,
        .funStart = 4,
        .playEnergy = 2,
        .energyBase = 10,
        .collapsePenalty = 5,
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

    // The middle of a prop's own box.
    // So a test aims at what the sink hit-tests, not at a guess.
    Point middleOf(const Prop prop)
    {
        const auto layout = layoutFor(kCanvas);
        const auto area = propBox(*layout, prop);

        return Point{
            .x = area.origin.x
                 + static_cast<std::int32_t>(area.size.width) / 2,
            .y = area.origin.y
                 + static_cast<std::int32_t>(area.size.height) / 2};
    }

    TickEvent pressAt(
        const InputEventCodec &codec,
        const Point at,
        const MouseButton button = MouseButton::Left)
    {
        return TickEvent{
            .tick = 0,
            .event = codec.encode(PointerButtonPressed{
                .button = button, .position = {.x = at.x, .y = at.y}})};
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

    TEST(PropSinkTest, APressOnTheBowlFeedsAHungryCompanion)
    {
        Pet pet(kQuick);
        const InputEventCodec codec;
        PropSink sink(pet, codec, kCanvas);

        pet.step();
        pet.step();
        ASSERT_TRUE(pet.hungry());

        sink.handle(pressAt(codec, middleOf(Prop::Bowl)));

        EXPECT_EQ(pet.meals(), 1U);
        EXPECT_EQ(pet.hunger(), 0U);
    }

    TEST(PropSinkTest, APressOnTheBallPlaysWithIt)
    {
        Pet pet(kQuick);
        const InputEventCodec codec;
        PropSink sink(pet, codec, kCanvas);

        sink.handle(pressAt(codec, middleOf(Prop::Ball)));

        EXPECT_EQ(pet.plays(), 1U);
        EXPECT_EQ(pet.energy(), kQuick.energyBase - kQuick.playEnergy);
    }

    TEST(PropSinkTest, APressOnTheNestSendsATiredCompanionToBed)
    {
        Pet pet(kQuick);
        const InputEventCodec codec;
        PropSink sink(pet, codec, kCanvas);

        // Three games spend six of ten, which is tired enough.
        // The fourth leaves two.
        for (int game = 0; game < 4; ++game)
        {
            sink.handle(pressAt(codec, middleOf(Prop::Ball)));
        }
        ASSERT_TRUE(pet.tired());

        sink.handle(pressAt(codec, middleOf(Prop::Nest)));

        EXPECT_EQ(pet.state(), PetState::Asleep);
    }

    // Sloppy aim has a price, which leaves the props worth aiming at.
    // So there is no press this sink drops on the floor.
    TEST(PropSinkTest, APressOnNothingInParticularIsAProd)
    {
        Pet pet(kQuick);
        const InputEventCodec codec;
        PropSink sink(pet, codec, kCanvas);

        const Point nowhere{.x = 128, .y = 64};
        ASSERT_FALSE(propAt(kCanvas, nowhere).has_value());

        sink.handle(pressAt(codec, nowhere));

        EXPECT_EQ(pet.pesters(), 1U);
        EXPECT_EQ(pet.meals(), 0U);
        EXPECT_EQ(pet.plays(), 0U);
    }

    TEST(PropSinkTest, OnlyALeftPressMeansAnything)
    {
        Pet pet(kQuick);
        const InputEventCodec codec;
        PropSink sink(pet, codec, kCanvas);

        pet.step();
        pet.step();

        const Point at = middleOf(Prop::Bowl);
        sink.handle(pressAt(codec, at, MouseButton::Right));
        sink.handle(TickEvent{
            .tick = 0,
            .event = codec.encode(PointerButtonReleased{
                .button = MouseButton::Left,
                .position = {.x = at.x, .y = at.y}})});
        sink.handle(TickEvent{
            .tick = 0,
            .event = codec.encode(KeyPressed{
                .key = antwika::input::Key::Space, .modifiers = {}})});

        EXPECT_EQ(pet.meals(), 0U);
        EXPECT_EQ(pet.pesters(), 0U);
    }

    TEST(PropSinkTest, AnEventThatIsNotInputAtAllIsIgnored)
    {
        Pet pet(kQuick);
        const InputEventCodec codec;
        PropSink sink(pet, codec, kCanvas);

        sink.handle(tick());
        sink.handle(other());

        EXPECT_EQ(pet.ticks(), 0U);
        EXPECT_EQ(pet.meals(), 0U);
        EXPECT_EQ(pet.pesters(), 0U);
    }

    TEST(RenderSinkTest, ATickDrawsAFrame)
    {
        const Pet pet(kQuick);
        const Lineage lineage;
        const antwika::i18n::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};
        NiceMock<MockRenderer> renderer;
        NiceMock<MockWindow> window;
        ON_CALL(window, isOpen()).WillByDefault(Return(true));
        ON_CALL(window, renderer()).WillByDefault(ReturnRef(renderer));

        EXPECT_CALL(renderer, present()).Times(1);

        RenderSink sink(window, scene, pet, lineage, kCanvas);
        sink.handle(tick());
    }

    TEST(RenderSinkTest, AClosedWindowAndANonTickDrawNothing)
    {
        const Pet pet(kQuick);
        const Lineage lineage;
        const antwika::i18n::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};
        NiceMock<MockWindow> window;
        ON_CALL(window, isOpen()).WillByDefault(Return(false));

        EXPECT_CALL(window, renderer()).Times(0);

        RenderSink sink(window, scene, pet, lineage, kCanvas);
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
