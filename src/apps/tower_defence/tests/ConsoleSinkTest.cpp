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
#include <antwika/i18n/Locale.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/testing/ScratchPath.hpp>

#include "antwika/tower_defence/Campaign.hpp"
#include "antwika/tower_defence/GridLayout.hpp"
#include "antwika/tower_defence/LevelTile.hpp"
#include "antwika/tower_defence/StateDump.hpp"
#include "antwika/tower_defence/TowerDefence.hpp"
#include "antwika/tower_defence/Wave.hpp"

using antwika::console::kConsoleAnimTicks;
using antwika::console::testing::keyAt;
using antwika::console::testing::kOpenTick;
using antwika::console::testing::pressAt;
using antwika::console::testing::stopAt;
using antwika::console::testing::typeText;
using antwika::event::mocks::MockEventSink;
using antwika::event::TickEvent;
using antwika::gfx::Size;
using antwika::input::InputEventCodec;
using antwika::input::Key;
using antwika::log::mocks::MockLogger;
using antwika::replay::ReplaySource;
using antwika::time::Tick;
using antwika::tower_defence::BattleConfig;
using antwika::tower_defence::BattleSummary;
using antwika::tower_defence::Campaign;
using antwika::tower_defence::CampaignConfig;
using antwika::tower_defence::Cell;
using antwika::tower_defence::cellRect;
using antwika::tower_defence::layoutFor;
using antwika::tower_defence::LevelPlan;
using antwika::tower_defence::MobKind;
using antwika::tower_defence::StateDump;
using antwika::tower_defence::Tile;
using antwika::tower_defence::Translator;
using antwika::tower_defence::Wave;
using antwika::tower_defence::WaveEntry;
using ::testing::NiceMock;

namespace
{
    constexpr Size kCanvas{.width = 960, .height = 720};

    CampaignConfig tinyCampaign()
    {
        return CampaignConfig{
            .seed = 3,
            .lives = 20,
            .levels = {LevelPlan{
                .level = {.width = 7, .height = 5, .wallSpacing = 3},
                .battle = BattleConfig{},
                .waves = {Wave{
                    .entries = {WaveEntry{MobKind::Grunt, 2}},
                    .spawnPeriodTicks = 3,
                    .gapTicks = 0}}}}};
    }

    [[nodiscard]] antwika::console::Snapshot readSnapshot(
        const std::string &path)
    {
        const antwika::console::SnapshotFormat format(
            {.magic = antwika::tower_defence::kStateDumpMagic,
             .version = antwika::tower_defence::kStateDumpVersion},
            "antwika tower defence state dump document",
            antwika::tower_defence::standardStateDumpMigrations);

        return format.read(path);
    }

    struct ConsoleHarness final
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> eventSink;
        InputEventCodec codec;
        Translator translator{antwika::i18n::kDefaultLocale};
        antwika::console::ConsolePicture consoleOverlay{kCanvas};

        BattleSummary run(
            ReplaySource &source,
            const Tick maxTicks,
            const std::string &dumpPath,
            const bool loadEnabled = true)
        {
            return antwika::tower_defence::bootstrap({
                .logger = logger,
                .eventSink = eventSink,
                .inputSource = source,
                .codec = codec,
                .translator = translator,
                .canvas = kCanvas,
                .campaign = tinyCampaign(),
                .maxTicks = maxTicks,
                .consoleOverlay = consoleOverlay,
                .consoleLoadEnabled = loadEnabled,
                .stateDumpPath = dumpPath});
        }
    };

    struct SplitCells final
    {
        antwika::gfx::Point under;
        antwika::gfx::Point below;
    };

    [[nodiscard]] SplitCells cellsAstrideTheSheet()
    {
        const Campaign probe(tinyCampaign());
        const auto &level = probe.battle().level();
        const auto layout =
            layoutFor(kCanvas, level.width, level.height);

        const auto sheet =
            static_cast<std::int32_t>(kCanvas.height / 2);

        SplitCells cells{};
        bool haveUnder = false;
        bool haveBelow = false;
        for (std::uint32_t y = 0; y < level.height; ++y)
        {
            for (std::uint32_t x = 0; x < level.width; ++x)
            {
                const Cell cell{.x = x, .y = y};
                if (level.at(cell) != Tile::Empty)
                {
                    continue;
                }

                const auto rect = cellRect(*layout, cell);
                const antwika::gfx::Point centre{
                    .x = rect.origin.x
                        + static_cast<std::int32_t>(rect.size.width / 2),
                    .y = rect.origin.y
                        + static_cast<std::int32_t>(
                            rect.size.height / 2)};

                if (!haveUnder && centre.y < sheet)
                {
                    cells.under = centre;
                    haveUnder = true;
                }
                if (!haveBelow && centre.y > sheet)
                {
                    cells.below = centre;
                    haveBelow = true;
                }
            }
        }

        EXPECT_TRUE(haveUnder);
        EXPECT_TRUE(haveBelow);
        return cells;
    }

    struct TowerDefenceConsoleTraits final
    {
        using Summary = BattleSummary;

        static Summary run(
            std::vector<TickEvent> script,
            const std::string &dumpPath,
            const bool loadEnabled)
        {
            script.push_back(stopAt(kOpenTick + 1));

            ReplaySource source(std::move(script));
            ConsoleHarness harness;

            return harness.run(source, 40, dumpPath, loadEnabled);
        }

        static const std::vector<std::string> &console(
            const Summary &summary)
        {
            return summary.console;
        }

        static void expectUntouched(const Summary &summary)
        {
            EXPECT_EQ(summary.towers, 0U);
        }

        static std::string scratchPrefix()
        {
            return "antwika_tower_defence_console.";
        }
    };
}

namespace antwika::console::conformance
{

    INSTANTIATE_TYPED_TEST_SUITE_P(
        TowerDefence, ConsoleContractTest, TowerDefenceConsoleTraits);

    INSTANTIATE_TYPED_TEST_SUITE_P(
        TowerDefence,
        ConsoleSnapshotRoundTripTest,
        TowerDefenceConsoleTraits);

}

TEST(ConsoleSinkTest, Run_PlacesNoTowerUnderTheSheet)
{
    const SplitCells cells = cellsAstrideTheSheet();

    ConsoleHarness harness;
    std::vector<TickEvent> events{keyAt(harness.codec, 1, Key::Grave)};

    events.push_back(pressAt(harness.codec, kOpenTick, cells.under));
    events.push_back(pressAt(harness.codec, kOpenTick, cells.below));
    events.push_back(stopAt(kOpenTick + 1));
    ReplaySource source(std::move(events));

    const auto summary = harness.run(source, 40, "unused.json");

    EXPECT_EQ(summary.towers, 1U);
}

TEST(ConsoleSinkTest, LoadState_ComesBackToTheDumpedInstant)
{
    const antwika::testing::ScratchFile file(
        "antwika_tower_defence_console_round_trip.json");
    const auto path = file.path().string();

    const SplitCells cells = cellsAstrideTheSheet();

    {
        ConsoleHarness harness;
        std::vector<TickEvent> events{
            pressAt(harness.codec, 1, cells.below),
            keyAt(harness.codec, 2, Key::Grave)};
        typeText(
            events, harness.codec, 2 + kConsoleAnimTicks, "dump_state");
        events.push_back(
            keyAt(harness.codec, 2 + kConsoleAnimTicks, Key::Enter));
        events.push_back(stopAt(3 + kConsoleAnimTicks));
        ReplaySource source(std::move(events));

        harness.run(source, 40, path);
    }

    const auto snapshot = readSnapshot(path);
    const StateDump dumped =
        antwika::tower_defence::stateDumpFromJson(snapshot.state);

    ASSERT_FALSE(dumped.campaign.battle.mobs.empty());
    ASSERT_EQ(dumped.campaign.battle.towers.size(), 1U);

    ConsoleHarness fresh;
    std::vector<TickEvent> events{keyAt(fresh.codec, 1, Key::Grave)};
    typeText(events, fresh.codec, kOpenTick, "load_state");
    events.push_back(keyAt(fresh.codec, kOpenTick, Key::Enter));
    typeText(events, fresh.codec, kOpenTick, "dump_state");
    events.push_back(keyAt(fresh.codec, kOpenTick, Key::Enter));
    events.push_back(stopAt(kOpenTick + 1));
    ReplaySource source(std::move(events));

    const auto summary = fresh.run(source, 40, path);

    const auto rewritten = readSnapshot(path);
    EXPECT_EQ(rewritten.state, snapshot.state);

    EXPECT_EQ(summary.towers, dumped.campaign.battle.towers.size());
    EXPECT_EQ(
        summary.console.back(), "dumped state to " + path);
}
