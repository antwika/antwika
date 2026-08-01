#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/mocks/MockEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/time/fakes/FakeSleeper.hpp>

#include "antwika/companion/Companion.hpp"
#include "antwika/companion/IPetStore.hpp"
#include "antwika/companion/Pet.hpp"
#include "antwika/companion/SaveFormatError.hpp"

using antwika::companion::CompanionConfig;
using antwika::companion::CompanionSummary;
using antwika::companion::IPetStore;
using antwika::companion::PetConfig;
using antwika::companion::PetMemory;
using antwika::companion::PetState;
using antwika::companion::SaveFormatError;
using antwika::companion::Saying;
using antwika::companion::storeIfLive;
using antwika::event::Event;
using antwika::event::mocks::MockEventSink;
using antwika::event::TickEvent;
using antwika::input::InputEventCodec;
using antwika::log::mocks::MockLogger;
using antwika::replay::ReplaySource;
using antwika::time::fakes::FakeSleeper;
using ::testing::_;
using ::testing::AnyNumber;
using ::testing::HasSubstr;
using ::testing::NiceMock;

namespace
{
    constexpr antwika::time::Tick kMaxTicks = 8;

    // A session of six ticks that then asks to stop.
    // Reaching the cap instead is EngineLoop throwing.
    // Which is a session that ends badly rather than one that ends.
    constexpr antwika::time::Tick kSessionTicks = 6;

    // The tick carrying the stop is dispatched like any other.
    // So the companion is stepped on it too.
    constexpr antwika::time::Tick kSteppedTicks = kSessionTicks + 1;

    std::vector<TickEvent> stopsOnItsOwn()
    {
        return {TickEvent{
            .tick = kSessionTicks,
            .event = Event{.name = antwika::engine::events::kStop}}};
    }

    constexpr PetConfig kBrisk{
        .dayTicks = 6,
        .nightTicks = 4,
        .hungerPeriodTicks = 1,
        .starvePeriodTicks = 1000,
        .restPeriodTicks = 1000,
        .hungerMax = 8,
        .hungerThreshold = 2,
        .feedRelief = 2,
        .feedJoy = 1,
        .disturbCost = 1,
        .happinessMax = 6,
        .happinessStart = 4};

    // A store with no filesystem behind it.
    // Which is the whole reason IPetStore exists.
    // A session cannot tell this from a real one.
    class FakePetStore final : public IPetStore
    {
    public:
        std::optional<PetMemory> held = std::nullopt;
        std::optional<PetMemory> written = std::nullopt;
        bool refusesToRead = false;
        bool breaksDown = false;
        bool refusesToWrite = false;
        int loads = 0;
        int saves = 0;

        std::optional<PetMemory> load() override
        {
            ++loads;

            if (breaksDown)
            {
                throw std::runtime_error("a store that is not working");
            }

            if (refusesToRead)
            {
                throw SaveFormatError("a companion that will not read");
            }

            return held;
        }

        void save(const PetMemory &memory) override
        {
            ++saves;

            if (refusesToWrite)
            {
                throw SaveFormatError("a companion that will not write");
            }

            written = memory;
        }
    };

    PetMemory lived()
    {
        return PetMemory{
            .ticks = 100,
            .state = PetState::Awake,
            .saying = Saying::Hello,
            .sayingTicksLeft = 3,
            .hunger = 5,
            .happiness = 6,
            .meals = 7,
            .disturbances = 2,
            .pesters = 1,
            .disturbed = false};
    }

    // The config a bootstrap needs but a test does not care about.
    // Assembled in one place, since every case below repeats it.
    CompanionConfig configFor(
        MockLogger &logger, IPetStore &store, ReplaySource &source,
        MockEventSink &events, FakeSleeper &sleeper,
        const InputEventCodec &codec)
    {
        return CompanionConfig{
            .logger = logger,
            .eventSink = events,
            .inputSource = source,
            .codec = codec,
            .sleeper = sleeper,
            .pet = kBrisk,
            .store = store,
            .maxTicks = kMaxTicks};
    }

    CompanionSummary runStored(MockLogger &logger, FakePetStore &store)
    {
        NiceMock<MockEventSink> events;
        FakeSleeper sleeper;
        ReplaySource source(stopsOnItsOwn());
        const InputEventCodec codec;

        return antwika::companion::bootstrap(
            configFor(logger, store, source, events, sleeper, codec));
    }

    TEST(SessionPersistenceTest, ASessionWithNoStoreKeepsNothing)
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> events;
        FakeSleeper sleeper;
        ReplaySource source(stopsOnItsOwn());
        const InputEventCodec codec;

        const CompanionSummary summary =
            antwika::companion::bootstrap(CompanionConfig{
                .logger = logger,
                .eventSink = events,
                .inputSource = source,
                .codec = codec,
                .sleeper = sleeper,
                .pet = kBrisk,
                .maxTicks = kMaxTicks});

        EXPECT_EQ(summary.ticks, kSteppedTicks);
    }

    TEST(SessionPersistenceTest, ANewCompanionIsStartedWhenThereIsNone)
    {
        NiceMock<MockLogger> logger;
        FakePetStore store;

        // The session logs its own lines, which are not the subject.
        EXPECT_CALL(logger, log(_, _)).Times(AnyNumber());
        EXPECT_CALL(
            logger, log(_, HasSubstr("No previous companion")));

        const CompanionSummary summary = runStored(logger, store);

        EXPECT_EQ(store.loads, 1);
        EXPECT_EQ(summary.ticks, kSteppedTicks);
        EXPECT_EQ(summary.meals, 0);
    }

    TEST(SessionPersistenceTest, ASessionCarriesOnFromTheOneBefore)
    {
        NiceMock<MockLogger> logger;
        FakePetStore store;
        store.held = lived();

        // The session logs its own lines, which are not the subject.
        EXPECT_CALL(logger, log(_, _)).Times(AnyNumber());
        EXPECT_CALL(logger, log(_, HasSubstr("Carrying on")));

        const CompanionSummary summary = runStored(logger, store);

        // The tick count carried over rather than starting again.
        EXPECT_EQ(summary.ticks, lived().ticks + kSteppedTicks);
        EXPECT_EQ(summary.meals, lived().meals);
        EXPECT_EQ(summary.disturbances, lived().disturbances);
        EXPECT_EQ(summary.pesters, lived().pesters);
    }

    TEST(SessionPersistenceTest, TheCompanionIsWrittenOutWhenTheRunEnds)
    {
        NiceMock<MockLogger> logger;
        FakePetStore store;
        store.held = lived();

        runStored(logger, store);

        EXPECT_EQ(store.saves, 1);
        ASSERT_TRUE(store.written.has_value());
        EXPECT_EQ(
            store.written->ticks, lived().ticks + kSteppedTicks);
        EXPECT_EQ(store.written->meals, lived().meals);
    }

    // A companion that will not read is said out loud and passed over.
    // Refusing to start would leave the app unusable.
    // Until somebody went and deleted a file by hand.
    TEST(SessionPersistenceTest, ACompanionThatWillNotReadStartsANewOne)
    {
        NiceMock<MockLogger> logger;
        FakePetStore store;
        store.refusesToRead = true;

        // The session logs its own lines, which are not the subject.
        EXPECT_CALL(logger, log(_, _)).Times(AnyNumber());
        EXPECT_CALL(logger, log(_, HasSubstr("could not be read")));

        const CompanionSummary summary = runStored(logger, store);

        EXPECT_EQ(summary.ticks, kSteppedTicks);
        EXPECT_EQ(store.saves, 1);
    }

    // The session is already over.
    // So the one thing left to say is that it was not kept.
    TEST(SessionPersistenceTest, ACompanionThatWillNotWriteIsReported)
    {
        NiceMock<MockLogger> logger;
        FakePetStore store;
        store.refusesToWrite = true;

        EXPECT_THROW(runStored(logger, store), SaveFormatError);
    }

    // Only a companion that will not read is stepped over.
    // A store that is broken is a failure rather than a first run.
    // So exactly one kind of failure is answered by carrying on.
    TEST(SessionPersistenceTest, AStoreThatIsBrokenEndsTheRun)
    {
        NiceMock<MockLogger> logger;
        FakePetStore store;
        store.breaksDown = true;

        EXPECT_THROW(runStored(logger, store), std::runtime_error);
    }

    // The replay rule, where a test can reach it.
    // An app's main() may hold no branch of its own.
    TEST(SessionPersistenceTest, StoreIfLive_OffersTheStoreToALiveRun)
    {
        FakePetStore store;

        const auto offered = storeIfLive(store, std::nullopt);

        ASSERT_TRUE(offered.has_value());
        EXPECT_EQ(&offered->get(), &store);
    }

    TEST(SessionPersistenceTest, StoreIfLive_OffersAReplayNothing)
    {
        FakePetStore store;

        EXPECT_FALSE(
            storeIfLive(store, std::optional<std::string>("demo.json"))
                .has_value());
    }
} // namespace
