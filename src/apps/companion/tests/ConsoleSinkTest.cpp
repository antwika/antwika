#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <antwika/console/ConsolePicture.hpp>
#include <antwika/console/ConsoleState.hpp>
#include <antwika/console/SnapshotFormat.hpp>
#include <antwika/console/conformance/ConsoleContractTest.hpp>
#include <antwika/console/conformance/ConsoleSnapshotRoundTripTest.hpp>
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

#include "PetFixtures.hpp"
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
using antwika::companion::tests::kCanvas;
using antwika::companion::tests::kBrisk;
using ::testing::NiceMock;

namespace
{
    constexpr Tick kMaxTicks = 60;



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

    struct ConsoleHarness final
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

    struct CompanionConsoleTraits final
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
}

namespace antwika::console::conformance
{

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Companion, ConsoleContractTest, CompanionConsoleTraits);

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Companion, ConsoleSnapshotRoundTripTest, CompanionConsoleTraits);

}

TEST(ConsoleSinkTest, Run_KeepsAPressUnderTheSheetAway)
{
    ConsoleHarness harness;

    const auto sheet =
        static_cast<std::int32_t>(kCanvas.height / 2);
    const Point under{.x = 128, .y = 64};
    const Point bowl = middleOf(Prop::Bowl);
    ASSERT_LT(under.y, sheet);
    ASSERT_GT(bowl.y, sheet);

    std::vector<TickEvent> events{keyAt(harness.codec, 1, Key::Grave)};

    events.push_back(pressAt(harness.codec, kOpenTick, under));
    events.push_back(pressAt(harness.codec, kOpenTick, bowl));
    events.push_back(stopAt(kOpenTick + 1));

    const auto summary =
        harness.run(std::move(events), "unused.json");

    EXPECT_EQ(summary.pesters, 0U);
    EXPECT_EQ(summary.meals, 1U);
}

TEST(ConsoleSinkTest, DumpState_WritesTheInstantAndSaysSo)
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

TEST(ConsoleSinkTest, LoadState_ComesBackToTheDumpedInstant)
{
    const antwika::testing::ScratchFile file(
        "antwika_companion_console_load.json");
    const auto path = file.path().string();

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

    ConsoleHarness fresh;
    std::vector<TickEvent> events{keyAt(fresh.codec, 1, Key::Grave)};
    typeText(events, fresh.codec, kOpenTick, "load_state");
    events.push_back(keyAt(fresh.codec, kOpenTick, Key::Enter));
    events.push_back(stopAt(kOpenTick + 1));

    const auto summary = fresh.run(std::move(events), path);

    EXPECT_EQ(summary.meals, 1U);
}
