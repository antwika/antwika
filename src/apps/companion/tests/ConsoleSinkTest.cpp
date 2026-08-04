#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <antwika/console/ConsolePicture.hpp>
#include <antwika/console/ConsoleState.hpp>
#include <antwika/console/SnapshotFormat.hpp>
#include <antwika/console/conformance/ConsoleContract.hpp>
#include <antwika/console/conformance/ConsoleSnapshotRoundTrip.hpp>
#include <antwika/console/testing/ConsoleScript.hpp>
#include <antwika/event/mocks/MockEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/testing/ScratchPath.hpp>
#include <antwika/time/fakes/FakeSleeper.hpp>

#include "antwika/companion/Companion.hpp"
#include "antwika/companion/CompanionSnapshotStore.hpp"
#include "antwika/companion/PetLayout.hpp"
#include "antwika/companion/PetSave.hpp"

using antwika::companion::CompanionSummary;
using antwika::companion::CompanionWiring;
using antwika::companion::layoutFor;
using antwika::companion::Pet;
using antwika::companion::PetConfig;
using antwika::companion::Prop;
using antwika::companion::propBox;
using antwika::console::kConsoleAnimTicks;
using antwika::console::testing::keyAt;
using antwika::console::testing::kOpenTick;
using antwika::console::testing::pressAt;
using antwika::console::testing::stopAt;
using antwika::console::testing::typeText;
using antwika::event::mocks::MockEventSink;
using antwika::event::TickEvent;
using antwika::gfx::Point;
using antwika::gfx::Size;
using antwika::input::InputEventCodec;
using antwika::input::Key;
using antwika::log::mocks::MockLogger;
using antwika::replay::ReplaySource;
using antwika::time::fakes::FakeSleeper;
using antwika::time::Tick;
using ::testing::NiceMock;

namespace
{
    constexpr Tick kMaxTicks = 60;

    constexpr Size kCanvas{.width = 256, .height = 256};

    // Hungry after two ticks, and nothing else moving at all.
    // So a session ends on what the scripted input did to it.
    constexpr PetConfig kBrisk{
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
        .hungerMax = 8,
        .hungerThreshold = 2,
        .feedRelief = 2,
        .funMax = 8,
        .funStart = 8,
        .playFun = 2,
        .playHunger = 1,
        .playEnergy = 2,
        .energyBase = 20,
        .collapsePenalty = 10,
        .happinessMax = 6,
        .happinessStart = 4};

    // The middle of a prop's own box.
    // So a test aims at what the sink hit-tests, not at a guess.
    [[nodiscard]] Point middleOf(const Prop prop)
    {
        const auto layout = layoutFor(kCanvas);
        const auto area = propBox(*layout, prop);

        return Point{
            .x = area.origin.x
                 + static_cast<std::int32_t>(area.size.width) / 2,
            .y = area.origin.y
                 + static_cast<std::int32_t>(area.size.height) / 2};
    }

    // A dump read back through the application's own envelope.
    [[nodiscard]] antwika::console::Snapshot readDump(
        const std::string &path)
    {
        const antwika::console::SnapshotFormat format(
            {.magic = antwika::companion::kStateDumpMagic,
             .version = antwika::companion::kStateDumpVersion},
            "antwika companion state dump document",
            antwika::companion::standardStateDumpMigrations);

        return format.read(path);
    }

    // RunIntegrationTest's harness, with the console turned on.
    struct ConsoleHarness
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> eventSink;
        InputEventCodec codec;
        FakeSleeper sleeper;
        antwika::console::ConsolePicture consoleOverlay{kCanvas};

        CompanionSummary run(
            std::vector<TickEvent> events,
            const std::string &dumpPath,
            bool loadEnabled = true)
        {
            ReplaySource source(std::move(events));

            return antwika::companion::bootstrap(CompanionWiring{
                .logger = logger,
                .eventSink = eventSink,
                .inputSource = source,
                .codec = codec,
                .sleeper = sleeper,
                .pet = kBrisk,
                .canvas = kCanvas,
                .consoleOverlay = consoleOverlay,
                .consoleLoadEnabled = loadEnabled,
                .stateDumpPath = dumpPath,
                .maxTicks = kMaxTicks});
        }
    };

    // This application's half of the shared console contract.
    struct CompanionConsoleTraits
    {
        using Summary = CompanionSummary;

        static Summary run(
            std::vector<TickEvent> script,
            const std::string &dumpPath,
            const bool loadEnabled)
        {
            script.push_back(stopAt(kOpenTick + 1));

            ConsoleHarness harness;

            return harness.run(
                std::move(script), dumpPath, loadEnabled);
        }

        static const std::vector<std::string> &console(
            const Summary &summary)
        {
            return summary.console;
        }

        static void expectUntouched(const Summary &summary)
        {
            EXPECT_EQ(summary.meals, 0U);
        }

        static std::string scratchPrefix()
        {
            return "antwika_companion_console.";
        }
    };
} // namespace

namespace antwika::console::conformance
{

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Companion, ConsoleContract, CompanionConsoleTraits);

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Companion, ConsoleSnapshotRoundTrip, CompanionConsoleTraits);

} // namespace antwika::console::conformance

TEST(ConsoleSinkTest, APressUnderTheSheetNeverReachesTheCompanion)
{
    ConsoleHarness harness;

    // The sheet reaches half way down the canvas.
    // The props stand along the ground, well below it.
    const auto sheet =
        static_cast<std::int32_t>(kCanvas.height / 2);
    const Point under{.x = 128, .y = 64};
    const Point bowl = middleOf(Prop::Bowl);
    ASSERT_LT(under.y, sheet);
    ASSERT_GT(bowl.y, sheet);

    std::vector<TickEvent> events{keyAt(harness.codec, 1, Key::Grave)};

    // Under the sheet the press is the console's, and prods nothing.
    // On the bowl below it, the hungry companion is still fed.
    events.push_back(pressAt(harness.codec, kOpenTick, under));
    events.push_back(pressAt(harness.codec, kOpenTick, bowl));
    events.push_back(stopAt(kOpenTick + 1));

    const auto summary =
        harness.run(std::move(events), "unused.json");

    EXPECT_EQ(summary.pesters, 0U);
    EXPECT_EQ(summary.meals, 1U);
}

TEST(ConsoleSinkTest, DumpStateWritesTheInstantAndSaysSo)
{
    const antwika::testing::ScratchFile file(
        "antwika_companion_console_dump.json");
    const auto path = file.path().string();

    ConsoleHarness harness;
    std::vector<TickEvent> events{
        pressAt(harness.codec, 5, middleOf(Prop::Bowl)),
        keyAt(harness.codec, 6, Key::Grave)};
    typeText(
        events, harness.codec, 6 + kConsoleAnimTicks, "dump_state");
    events.push_back(
        keyAt(harness.codec, 6 + kConsoleAnimTicks, Key::Enter));
    events.push_back(stopAt(7 + kConsoleAnimTicks));

    const auto summary = harness.run(std::move(events), path);

    const auto dumped = readDump(path);
    const auto memory =
        antwika::companion::companionMemoryFromJson(dumped.state);
    const Pet restored(kBrisk, memory.pet);

    EXPECT_EQ(restored.meals(), 1U);
    EXPECT_EQ(
        dumped.console,
        (std::vector<std::string>{
            "> dump_state", "dumped state to " + path}));
    EXPECT_EQ(summary.console, dumped.console);
}

TEST(ConsoleSinkTest, LoadStateComesBackToTheDumpedInstant)
{
    const antwika::testing::ScratchFile file(
        "antwika_companion_console_load.json");
    const auto path = file.path().string();

    // One session feeds the companion, then dumps itself.
    {
        ConsoleHarness harness;
        std::vector<TickEvent> events{
            pressAt(harness.codec, 5, middleOf(Prop::Bowl)),
            keyAt(harness.codec, 6, Key::Grave)};
        typeText(
            events, harness.codec, 6 + kConsoleAnimTicks, "dump_state");
        events.push_back(
            keyAt(harness.codec, 6 + kConsoleAnimTicks, Key::Enter));
        events.push_back(stopAt(7 + kConsoleAnimTicks));

        harness.run(std::move(events), path);
    }

    // A fresh session loads it mid-run and continues from there.
    ConsoleHarness fresh;
    std::vector<TickEvent> events{keyAt(fresh.codec, 1, Key::Grave)};
    typeText(events, fresh.codec, kOpenTick, "load_state");
    events.push_back(keyAt(fresh.codec, kOpenTick, Key::Enter));
    events.push_back(stopAt(kOpenTick + 1));

    const auto summary = fresh.run(std::move(events), path);

    // What the console history carries over is the contract's.
    // That the fed companion came back with it is this file's.
    EXPECT_EQ(summary.meals, 1U);
}
