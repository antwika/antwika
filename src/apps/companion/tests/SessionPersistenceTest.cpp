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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

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
#include <antwika/time/fakes/FakeSleeper.hpp>

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

    // Nothing at all happens to a companion in eight ticks here.
    // So a session ends on what it started from plus the ticks.
    constexpr PetConfig kBrisk{
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
        .hungerMax = 8,
        .hungerThreshold = 2,
        .feedRelief = 2,
        .funMax = 8,
        .funStart = 8,
        .playEnergy = 2,
        .energyBase = 20,
        .collapsePenalty = 10,
        .happinessMax = 6,
        .happinessStart = 4};

    // Removes its backing file on scope exit.
    // That way a failing assertion leaves no stray temp files behind.
    class ScratchFile
    {
    public:
        explicit ScratchFile(std::string_view name)
            : path(
                  std::filesystem::temp_directory_path()
                  // The pid keeps parallel ctest runs apart.
                  // Each case is its own process under ctest -j.
                  // Two cases on one fixed name raced here.
                  // See game/tests/ScratchDirectory.hpp.
                  / (std::string(name) + "." + std::to_string(::getpid())))
        {
        }

        ~ScratchFile()
        {
            // The error_code overload, not the throwing one.
            // A destructor is implicitly noexcept.
            std::error_code ignored;
            std::filesystem::remove(path, ignored);
        }

        ScratchFile(const ScratchFile &) = delete;
        ScratchFile(ScratchFile &&) = delete;
        ScratchFile &operator=(const ScratchFile &) = delete;
        ScratchFile &operator=(ScratchFile &&) = delete;

        [[nodiscard]] std::string string() const
        {
            return path.string();
        }

    private:
        std::filesystem::path path;
    };

    // A store with no filesystem behind it.
    // Which is the whole reason IPetStore exists.
    // A session cannot tell this from a real one.
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

    // The config a bootstrap needs but a test does not care about.
    // Assembled in one place, since every case below repeats it.
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
            antwika::companion::bootstrap(CompanionWiring{
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
        EXPECT_CALL(logger, log(_, HasSubstr("No previous companion")));

        const CompanionSummary summary = runStored(logger, store);

        EXPECT_EQ(store.loads, 1);
        EXPECT_EQ(summary.ticks, kSteppedTicks);
        EXPECT_EQ(summary.meals, 0U);
        EXPECT_EQ(summary.generation, 1U);
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
        EXPECT_EQ(summary.ticks, lived().pet.ticks + kSteppedTicks);
        EXPECT_EQ(summary.meals, lived().pet.meals);
        EXPECT_EQ(summary.plays, lived().pet.plays);
        EXPECT_EQ(summary.disturbances, lived().pet.disturbances);
        EXPECT_EQ(summary.pesters, lived().pet.pesters);
    }

    // The record is the file's rather than the companion's.
    // So it survives whatever happened to the one in front of it.
    TEST(SessionPersistenceTest, TheRecordBehindTheCompanionCarriesOnToo)
    {
        NiceMock<MockLogger> logger;
        FakePetStore store;
        store.held = lived();

        const CompanionSummary summary = runStored(logger, store);

        EXPECT_EQ(summary.generation, lived().lineage.generation);

        // The session's own age beat what the file remembered.
        EXPECT_EQ(summary.bestTicks, lived().pet.ticks + kSteppedTicks);
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
            store.written->pet.ticks, lived().pet.ticks + kSteppedTicks);
        EXPECT_EQ(store.written->pet.meals, lived().pet.meals);
        EXPECT_EQ(
            store.written->lineage.generation,
            lived().lineage.generation);
        EXPECT_EQ(
            store.written->lineage.bestTicks,
            lived().pet.ticks + kSteppedTicks);
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

    // A companion no session could be in is refused where it is read.
    // Rather than in RestoreSink, a tick into the run.
    // Which is why openSession() builds one and throws it away.
    TEST(SessionPersistenceTest, ACompanionNoSessionCouldBeInIsRefused)
    {
        NiceMock<MockLogger> logger;
        FakePetStore store;
        store.held = lived();
        store.held->pet.hunger = kBrisk.hungerMax + 1;

        EXPECT_CALL(logger, log(_, _)).Times(AnyNumber());
        EXPECT_CALL(logger, log(_, HasSubstr("could not be read")));

        const CompanionSummary summary = runStored(logger, store);

        EXPECT_EQ(summary.ticks, kSteppedTicks);
        EXPECT_EQ(summary.generation, 1U);
    }

    // A lineage no build could have written is refused as a pet is.
    // And it takes the same route out: a warning, and a new one.
    TEST(SessionPersistenceTest, ALineageThatWillNotReadStartsANewOne)
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

    // The session is already over.
    // So the one thing left to say is that it was not kept.
    TEST(SessionPersistenceTest, ACompanionThatWillNotWriteIsReported)
    {
        NiceMock<MockLogger> logger;
        FakePetStore store;
        store.refusesToWrite = true;

        EXPECT_THROW(runStored(logger, store), SaveFormatError);
    }

    // A balance no session could run on is this build's mistake.
    // Not a fact about somebody's file, which is the other type.
    // So it goes past the catch above rather than into it.
    // And the read of a perfectly good companion ends the run.
    TEST(SessionPersistenceTest, ABalanceNoSessionCouldRunOnEndsTheRun)
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> events;
        FakeSleeper sleeper;
        ReplaySource source(stopsOnItsOwn());
        const InputEventCodec codec;
        FakePetStore store;
        store.held = lived();

        PetConfig unbalanced = kBrisk;
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

    // What a `--record` run writes out, as a `--replay` reads it.
    // Through the real pair rather than straight off the recorder.
    // The filtering those do -- engine.tick above all -- is the point.
    // A round trip skipping it would prove something no run ever does.
    std::vector<TickEvent> throughAFile(std::vector<TickEvent> recorded)
    {
        const ScratchFile file{"antwika_companion_replay.json"};
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

        // No store at all, which is exactly what a `--replay` gets.
        // storeIfLive() withholds it.
        // So nothing here can reach the companion on this machine.
        return antwika::companion::bootstrap(CompanionWiring{
            .logger = logger,
            .eventSink = sink,
            .inputSource = source,
            .codec = codec,
            .sleeper = sleeper,
            .pet = kBrisk,
            .maxTicks = kMaxTicks});
    }

    // The requirement this project exists for, at the seam that broke it.
    // A `--record` run began on the companion a file was holding.
    // Its recording carried only the presses.
    // So replaying it started from a brand new animal.
    // And every press then meant something else.
    // The companion now travels as an event ahead of the first tick.
    TEST(SessionPersistenceTest, ARecordedSessionReplaysToTheSameAnimal)
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

        ASSERT_EQ(live.ticks, lived().pet.ticks + kSteppedTicks)
            << "the live run did not carry on from the file at all";

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

    // Only what came from outside the program.
    // Which here is the file's companion and the source's events.
    TEST(SessionPersistenceTest, ARecordingCarriesTheCompanionJustOnce)
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

    // A session with nothing to carry on from records no companion.
    // The recording then starts a new one, exactly as the run did.
    TEST(SessionPersistenceTest, ANewCompanionIsNotAnnouncedAtAll)
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

        for (const auto &event : recorder.getEvents())
        {
            EXPECT_NE(
                event.event.name,
                antwika::companion::events::kRestore);
        }
    }
} // namespace
