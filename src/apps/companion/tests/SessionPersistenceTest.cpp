#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>
#include <unistd.h>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/mocks/MockEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/event/TickEventRecorder.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/replay/ReplayCli.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/testing/ScratchPath.hpp>
#include <antwika/time/fakes/FakeSleeper.hpp>

#include "PetFixtures.hpp"
#include "antwika/companion/Companion.hpp"
#include "antwika/companion/CompanionError.hpp"
#include "antwika/companion/CompanionMemory.hpp"
#include "antwika/companion/Events.hpp"
#include "antwika/companion/IPetStore.hpp"
#include "antwika/companion/Pet.hpp"
#include "antwika/companion/SaveFormatError.hpp"

using antwika::companion::CompanionWiring;
using antwika::companion::CompanionError;
using antwika::companion::CompanionMemory;
using antwika::companion::CompanionSummary;
using antwika::companion::IPetStore;
using antwika::companion::LineageMemory;
using antwika::companion::PetConfig;
using antwika::companion::PetMemory;
using antwika::companion::PetState;
using antwika::companion::SaveFormatError;
using antwika::companion::Saying;
using antwika::companion::storeIfLive;
using antwika::event::Event;
using antwika::event::mocks::MockEventSink;
using antwika::event::TickEvent;
using antwika::event::TickEventRecorder;
using antwika::input::InputEventCodec;
using antwika::log::mocks::MockLogger;
using antwika::replay::ReplaySource;
using antwika::time::fakes::FakeSleeper;
using antwika::companion::tests::kUnhurried;
using antwika::companion::tests::lived;
using ::testing::_;
using ::testing::AnyNumber;
using ::testing::HasSubstr;
using ::testing::NiceMock;

namespace
{
    constexpr antwika::time::Tick kMaxTicks = 8;

    constexpr antwika::time::Tick kSessionTicks = 6;

    constexpr antwika::time::Tick kSteppedTicks = kSessionTicks + 1;

    std::vector<TickEvent> stopsOnItsOwn()
    {
        return {TickEvent{
            .tick = kSessionTicks,
            .event = Event{.name = antwika::engine::events::kStop}}};
    }

    class FakePetStore final : public IPetStore
    {
    public:
        std::optional<CompanionMemory> held = std::nullopt;
        std::optional<CompanionMemory> written = std::nullopt;
        bool refusesToRead = false;
        bool breaksDown = false;
        bool refusesToWrite = false;
        int loads = 0;
        int saves = 0;

        std::optional<CompanionMemory> load() override
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

        void save(const CompanionMemory &memory) override
        {
            ++saves;

            if (refusesToWrite)
            {
                throw SaveFormatError("a companion that will not write");
            }

            written = memory;
        }
    };


    CompanionWiring configFor(
        MockLogger &logger, IPetStore &store, ReplaySource &source,
        MockEventSink &events, FakeSleeper &sleeper,
        const InputEventCodec &codec)
    {
        return CompanionWiring{
            .logger = logger,
            .eventSink = events,
            .inputSource = source,
            .codec = codec,
            .sleeper = sleeper,
            .pet = kUnhurried,
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

    TEST(SessionPersistenceTest, Run_KeepsNothingWithoutAStore)
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> events;
        FakeSleeper sleeper;
        ReplaySource source(stopsOnItsOwn());
        const InputEventCodec codec;

        const CompanionSummary summary =
            antwika::companion::bootstrap(CompanionWiring{
                .logger = logger,
                .eventSink = events,
                .inputSource = source,
                .codec = codec,
                .sleeper = sleeper,
                .pet = kUnhurried,
                .maxTicks = kMaxTicks});

        EXPECT_EQ(summary.ticks, kSteppedTicks);
    }

    TEST(SessionPersistenceTest, RunStored_StartsANewCompanionWhenThereIsNone)
    {
        NiceMock<MockLogger> logger;
        FakePetStore store;

        EXPECT_CALL(logger, log(_, _)).Times(AnyNumber());
        EXPECT_CALL(logger, log(_, HasSubstr("No previous companion")));

        const CompanionSummary summary = runStored(logger, store);

        EXPECT_EQ(store.loads, 1);
        EXPECT_EQ(summary.ticks, kSteppedTicks);
        EXPECT_EQ(summary.meals, 0U);
        EXPECT_EQ(summary.generation, 1U);
    }

    TEST(SessionPersistenceTest, RunStored_CarriesOnFromTheSessionBefore)
    {
        NiceMock<MockLogger> logger;
        FakePetStore store;
        store.held = lived();

        EXPECT_CALL(logger, log(_, _)).Times(AnyNumber());
        EXPECT_CALL(logger, log(_, HasSubstr("Carrying on")));

        const CompanionSummary summary = runStored(logger, store);

        EXPECT_EQ(summary.ticks, lived().pet.ticks + kSteppedTicks);
        EXPECT_EQ(summary.meals, lived().pet.meals);
        EXPECT_EQ(summary.plays, lived().pet.plays);
        EXPECT_EQ(summary.disturbances, lived().pet.disturbances);
        EXPECT_EQ(summary.pesters, lived().pet.pesters);
    }

    TEST(SessionPersistenceTest, RunStored_CarriesTheRecordOnToo)
    {
        NiceMock<MockLogger> logger;
        FakePetStore store;
        store.held = lived();

        const CompanionSummary summary = runStored(logger, store);

        EXPECT_EQ(summary.generation, lived().lineage.generation);

        EXPECT_EQ(summary.bestTicks, lived().pet.ticks + kSteppedTicks);
    }

    TEST(SessionPersistenceTest, RunStored_WritesTheCompanionOutAtTheEnd)
    {
        NiceMock<MockLogger> logger;
        FakePetStore store;
        store.held = lived();

        runStored(logger, store);

        EXPECT_EQ(store.saves, 1);
        ASSERT_TRUE(store.written.has_value());
        EXPECT_EQ(
            store.written->pet.ticks, lived().pet.ticks + kSteppedTicks);
        EXPECT_EQ(store.written->pet.meals, lived().pet.meals);
        EXPECT_EQ(
            store.written->lineage.generation,
            lived().lineage.generation);
        EXPECT_EQ(
            store.written->lineage.bestTicks,
            lived().pet.ticks + kSteppedTicks);
    }

    TEST(SessionPersistenceTest, RunStored_StartsAnewWhenTheReadFails)
    {
        NiceMock<MockLogger> logger;
        FakePetStore store;
        store.refusesToRead = true;

        EXPECT_CALL(logger, log(_, _)).Times(AnyNumber());
        EXPECT_CALL(logger, log(_, HasSubstr("could not be read")));

        const CompanionSummary summary = runStored(logger, store);

        EXPECT_EQ(summary.ticks, kSteppedTicks);
        EXPECT_EQ(store.saves, 1);
    }

    TEST(SessionPersistenceTest, RunStored_RefusesAnImpossibleCompanion)
    {
        NiceMock<MockLogger> logger;
        FakePetStore store;
        store.held = lived();
        store.held->pet.hunger = kUnhurried.hungerMax + 1;

        EXPECT_CALL(logger, log(_, _)).Times(AnyNumber());
        EXPECT_CALL(logger, log(_, HasSubstr("could not be read")));

        const CompanionSummary summary = runStored(logger, store);

        EXPECT_EQ(summary.ticks, kSteppedTicks);
        EXPECT_EQ(summary.generation, 1U);
    }

    TEST(SessionPersistenceTest, RunStored_StartsAnewWhenTheLineageFails)
    {
        NiceMock<MockLogger> logger;
        FakePetStore store;
        store.held = lived();
        store.held->lineage.generation = 0;

        EXPECT_CALL(logger, log(_, _)).Times(AnyNumber());
        EXPECT_CALL(logger, log(_, HasSubstr("could not be read")));

        const CompanionSummary summary = runStored(logger, store);

        EXPECT_EQ(summary.ticks, kSteppedTicks);
        EXPECT_EQ(summary.generation, 1U);
    }

    TEST(SessionPersistenceTest, RunStored_ReportsAFailedWrite)
    {
        NiceMock<MockLogger> logger;
        FakePetStore store;
        store.refusesToWrite = true;

        EXPECT_THROW(runStored(logger, store), SaveFormatError);
    }

    TEST(SessionPersistenceTest, Run_EndsOnABalanceNoSessionCouldRunOn)
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> events;
        FakeSleeper sleeper;
        ReplaySource source(stopsOnItsOwn());
        const InputEventCodec codec;
        FakePetStore store;
        store.held = lived();

        PetConfig unbalanced = kUnhurried;
        unbalanced.hungerPeriodTicks = 0;

        EXPECT_THROW(
            antwika::companion::bootstrap(CompanionWiring{
                .logger = logger,
                .eventSink = events,
                .inputSource = source,
                .codec = codec,
                .sleeper = sleeper,
                .pet = unbalanced,
                .store = store,
                .maxTicks = kMaxTicks}),
            CompanionError);
    }

    TEST(SessionPersistenceTest, RunStored_EndsTheRunOnABrokenStore)
    {
        NiceMock<MockLogger> logger;
        FakePetStore store;
        store.breaksDown = true;

        EXPECT_THROW(runStored(logger, store), std::runtime_error);
    }

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

    std::vector<TickEvent> throughAFile(std::vector<TickEvent> recorded)
    {
        const antwika::testing::ScratchFile file{
            "antwika_companion_replay.json"};
        antwika::replay::saveReplayFile(
            std::move(recorded), file.string());

        return antwika::replay::loadReplayFile(file.string());
    }

    CompanionSummary replaySession(std::vector<TickEvent> events)
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> sink;
        FakeSleeper sleeper;
        ReplaySource source(std::move(events));
        const InputEventCodec codec;

        return antwika::companion::bootstrap(CompanionWiring{
            .logger = logger,
            .eventSink = sink,
            .inputSource = source,
            .codec = codec,
            .sleeper = sleeper,
            .pet = kUnhurried,
            .maxTicks = kMaxTicks});
    }

    TEST(SessionPersistenceTest, Replay_ReachesTheSameAnimal)
    {
        NiceMock<MockLogger> logger;
        FakePetStore store;
        store.held = lived();

        NiceMock<MockEventSink> events;
        FakeSleeper sleeper;
        ReplaySource source(stopsOnItsOwn());
        const InputEventCodec codec;
        TickEventRecorder recorder;

        auto config =
            configFor(logger, store, source, events, sleeper, codec);
        config.replayRecorder = recorder;

        const CompanionSummary live =
            antwika::companion::bootstrap(config);

        ASSERT_EQ(live.ticks, lived().pet.ticks + kSteppedTicks);

        const CompanionSummary replayed =
            replaySession(throughAFile(recorder.getEvents()));

        EXPECT_EQ(replayed.ticks, live.ticks);
        EXPECT_EQ(replayed.day, live.day);
        EXPECT_EQ(replayed.hunger, live.hunger);
        EXPECT_EQ(replayed.fun, live.fun);
        EXPECT_EQ(replayed.happiness, live.happiness);
        EXPECT_EQ(replayed.energy, live.energy);
        EXPECT_EQ(replayed.energyCeiling, live.energyCeiling);
        EXPECT_EQ(replayed.meals, live.meals);
        EXPECT_EQ(replayed.plays, live.plays);
        EXPECT_EQ(replayed.disturbances, live.disturbances);
        EXPECT_EQ(replayed.pesters, live.pesters);
        EXPECT_EQ(replayed.collapses, live.collapses);
        EXPECT_EQ(replayed.generation, live.generation);
        EXPECT_EQ(replayed.bestTicks, live.bestTicks);
        EXPECT_EQ(replayed.perished, live.perished);
    }

    TEST(SessionPersistenceTest, Recording_CarriesTheCompanionJustOnce)
    {
        NiceMock<MockLogger> logger;
        FakePetStore store;
        store.held = lived();

        NiceMock<MockEventSink> events;
        FakeSleeper sleeper;
        ReplaySource source(stopsOnItsOwn());
        const InputEventCodec codec;
        TickEventRecorder recorder;

        auto config =
            configFor(logger, store, source, events, sleeper, codec);
        config.replayRecorder = recorder;

        antwika::companion::bootstrap(config);

        const auto saved = throughAFile(recorder.getEvents());
        std::size_t announcements = 0;

        for (const auto &event : saved)
        {
            EXPECT_NE(
                event.event.name, antwika::engine::events::kTick);

            if (event.event.name
                == antwika::companion::events::kRestore)
            {
                ++announcements;
                EXPECT_EQ(event.tick, 0U);
            }
        }

        EXPECT_EQ(announcements, 1U);
    }

    TEST(SessionPersistenceTest, Run_AnnouncesNothingForANewCompanion)
    {
        NiceMock<MockLogger> logger;
        FakePetStore store;

        NiceMock<MockEventSink> events;
        FakeSleeper sleeper;
        ReplaySource source(stopsOnItsOwn());
        const InputEventCodec codec;
        TickEventRecorder recorder;

        auto config =
            configFor(logger, store, source, events, sleeper, codec);
        config.replayRecorder = recorder;

        antwika::companion::bootstrap(config);

        ASSERT_FALSE(recorder.getEvents().empty());

        for (const auto &event : recorder.getEvents())
        {
            EXPECT_NE(
                event.event.name,
                antwika::companion::events::kRestore);
        }
    }
}
