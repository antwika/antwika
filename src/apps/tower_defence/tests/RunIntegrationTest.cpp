#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/mocks/MockEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/event/TickEventRecorder.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/replay/ReplaySource.hpp>

#include "antwika/tower_defence/Campaign.hpp"
#include "antwika/tower_defence/GridLayout.hpp"
#include "antwika/tower_defence/IScoreStore.hpp"
#include "antwika/tower_defence/LevelTile.hpp"
#include "antwika/tower_defence/Messages.hpp"
#include "antwika/tower_defence/MobKind.hpp"
#include "antwika/tower_defence/ScoreFormatError.hpp"
#include "antwika/tower_defence/TowerDefence.hpp"

using antwika::event::Event;
using antwika::event::mocks::MockEventSink;
using antwika::event::TickEvent;
using antwika::event::TickEventRecorder;
using antwika::gfx::Size;
using antwika::input::InputEventCodec;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::log::mocks::MockLogger;
using antwika::replay::ReplaySource;
using antwika::tower_defence::BattleConfig;
using antwika::tower_defence::BattleSummary;
using antwika::tower_defence::Campaign;
using antwika::tower_defence::CampaignConfig;
using antwika::tower_defence::CampaignPhase;
using antwika::tower_defence::Cell;
using antwika::tower_defence::cellRect;
using antwika::tower_defence::HighScore;
using antwika::tower_defence::IScoreStore;
using antwika::tower_defence::layoutFor;
using antwika::tower_defence::LevelPlan;
using antwika::tower_defence::MobKind;
using antwika::tower_defence::ScoreFormatError;
using antwika::tower_defence::storeIfLive;
using antwika::tower_defence::summaryLine;
using antwika::tower_defence::TowerDefenceConfig;
using antwika::tower_defence::Translator;
using antwika::tower_defence::Wave;
using antwika::tower_defence::WaveEntry;
using ::testing::NiceMock;

namespace
{
    constexpr Size kCanvas{.width = 960, .height = 720};
    constexpr std::uint32_t kWidth = 7;
    constexpr std::uint32_t kHeight = 5;
    constexpr antwika::time::Tick kMaxTicks = 60;

    // One small level whose wave outlasts the run's stop event.
    // So the towers a click built are still standing at the end.
    // And so the solver is not what these cases cost.
    // A reach of nothing is a gun that can never fire.
    CampaignConfig campaignConfig(
        const std::uint32_t lives = 12, const std::uint32_t reach = 9)
    {
        const Wave wave{
            .entries = {WaveEntry{MobKind::Grunt, 6}},
            .spawnPeriodTicks = 6,
            .gapTicks = 0};
        const LevelPlan plan{
            .level =
                {.width = kWidth, .height = kHeight, .wallSpacing = 3},
            .battle =
                BattleConfig{
                    .towerRangeSquared = reach, .towerDamage = 3},
            .waves = {wave}};

        return CampaignConfig{
            .seed = 3, .lives = lives, .levels = {plan}};
    }

    // Answers from memory, so nothing here opens a file.
    class FakeScoreStore final : public IScoreStore
    {
    public:
        std::optional<HighScore> kept;
        std::optional<HighScore> written;
        bool refuseToRead = false;

        std::optional<HighScore> load() override
        {
            if (refuseToRead)
            {
                throw ScoreFormatError("nothing readable here");
            }
            return kept;
        }

        void save(const HighScore &score) override
        {
            written = score;
        }
    };

    // The generated level decides where the road runs.
    // So the cell built on is read off a probe of the same config.
    // It is one beside the road, the only kind worth building on.
    Cell aBuildableCell()
    {
        const Campaign probe(campaignConfig());
        const auto &level = probe.battle().level();

        for (const Cell &on : level.path)
        {
            const Cell around[] = {
                {.x = on.x, .y = on.y + 1},
                {.x = on.x + 1, .y = on.y},
                {.x = on.x, .y = on.y - 1},
                {.x = on.x - 1, .y = on.y}};

            for (const Cell &cell : around)
            {
                if (cell.x < level.width && cell.y < level.height
                    && level.at(cell)
                        == antwika::tower_defence::Tile::Empty)
                {
                    return cell;
                }
            }
        }
        return Cell{};
    }

    // One press on open ground and one on the score bar.
    // The test asserts what happened rather than assuming it built.
    std::vector<TickEvent> script()
    {
        const InputEventCodec codec;
        const auto layout = layoutFor(kCanvas, kWidth, kHeight);
        const auto rect = cellRect(*layout, aBuildableCell());

        return {
            TickEvent{
                .tick = 2,
                .event = codec.encode(PointerButtonPressed{
                    .button = MouseButton::Left,
                    .position = {
                        .x = rect.origin.x + 4,
                        .y = rect.origin.y + 4}})},
            TickEvent{
                .tick = 3,
                .event = codec.encode(PointerButtonPressed{
                    .button = MouseButton::Left,
                    .position = {.x = 20, .y = 8}})},
            TickEvent{
                .tick = 40,
                .event = Event{
                    .name = antwika::engine::events::kStop}}};
    }

    // What the watcher below saw, kept outside it.
    // bootstrap() owns the sink and destroys it before returning.
    struct WatchedTicks
    {
        std::uint64_t ticks = 0;
        std::size_t towers = 0;
        std::size_t commands = 0;
    };

    // Stands in for the renderer main.cpp hands bootstrap().
    // It is built over the Campaign and ScoreOverlay the run owns.
    // Neither exists before a level has been generated.
    // It reads the same two things RenderSink does.
    // So what it sees is what a frame would be drawn from.
    class FinishedTickWatcher final
        : public antwika::event::ITickEventSink
    {
    public:
        FinishedTickWatcher(
            const Campaign &campaign,
            const antwika::tower_defence::ScoreOverlay &overlay,
            WatchedTicks &seen)
            : campaign(campaign), overlay(overlay), seen(seen)
        {
        }

        void handle(const TickEvent &event) override
        {
            if (event.event.name != antwika::engine::events::kTick)
            {
                return;
            }
            ++seen.ticks;
            seen.towers = campaign.battle().towers().size();
            seen.commands = overlay.commands().size();
        }

    private:
        const Campaign &campaign;
        const antwika::tower_defence::ScoreOverlay &overlay;
        WatchedTicks &seen;
    };

    BattleSummary runOnce(
        IScoreStore *store = nullptr,
        const std::uint32_t lives = 12,
        const std::uint32_t reach = 9)
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> eventSink;
        const InputEventCodec codec;
        const Translator translator{antwika::i18n::kDefaultLocale};
        ReplaySource source(script());

        TowerDefenceConfig config{
            .logger = logger,
            .eventSink = eventSink,
            .inputSource = source,
            .codec = codec,
            .translator = translator,
            .canvas = kCanvas,
            .campaign = campaignConfig(lives, reach),
            .maxTicks = kMaxTicks};

        if (store != nullptr)
        {
            config.scoreStore = *store;
        }

        return antwika::tower_defence::bootstrap(config);
    }
} // namespace

// The requirement this project exists for, for this app.
// A run is driven entirely by what a source hands it.
// Nothing about the level, the waves or the mobs is recorded.
// So running the same input twice has to land on the same state.
TEST(RunIntegrationTest, TheSameInputReachesTheSameStateTwice)
{
    const BattleSummary first = runOnce();
    const BattleSummary second = runOnce();

    EXPECT_EQ(first.score, second.score);
    EXPECT_EQ(first.lives, second.lives);
    EXPECT_EQ(first.ticks, second.ticks);
    EXPECT_EQ(first.towers, second.towers);
    EXPECT_EQ(first.pathLength, second.pathLength);
    EXPECT_EQ(first.level, second.level);
    EXPECT_EQ(first.wavesReleased, second.wavesReleased);
    EXPECT_EQ(first.phase, second.phase);
}

TEST(RunIntegrationTest, AStopEventEndsTheRunBeforeTheCap)
{
    const BattleSummary summary = runOnce();
    EXPECT_GT(summary.ticks, 0U);
    EXPECT_LT(summary.ticks, kMaxTicks);
    EXPECT_GT(summary.pathLength, 2U);
}

// One click landed on open ground and one on the score bar.
// Only the first can have built anything.
TEST(RunIntegrationTest, AClickOnTheBarBuildsNothingButAClickOnGroundDoes)
{
    const BattleSummary summary = runOnce();
    EXPECT_EQ(summary.towers, 1U);
}

// main.cpp hangs the renderer off the run with the extraSink factory.
// A sink drawing the campaign needs state bootstrap() makes itself.
// The sink must be registered last.
// So what it reads is the state the tick ended with.
TEST(RunIntegrationTest, TheExtraSinkSeesEveryFinishedTick)
{
    NiceMock<MockLogger> logger;
    NiceMock<MockEventSink> eventSink;
    const InputEventCodec codec;
    const Translator translator{antwika::i18n::kDefaultLocale};
    ReplaySource source(script());

    WatchedTicks seen;
    const BattleSummary summary =
        antwika::tower_defence::bootstrap(TowerDefenceConfig{
            .logger = logger,
            .eventSink = eventSink,
            .inputSource = source,
            .codec = codec,
            .translator = translator,
            .canvas = kCanvas,
            .campaign = campaignConfig(),
            .maxTicks = kMaxTicks,
            .extraSink =
                [&seen](
                    const Campaign &campaign,
                    const antwika::tower_defence::ScoreOverlay &overlay)
            {
                return std::make_unique<FinishedTickWatcher>(
                    campaign, overlay, seen);
            }});

    EXPECT_EQ(seen.ticks, summary.ticks);

    // The tower the ground click built.
    // And the bar ScoreSink described before this sink ran.
    EXPECT_EQ(seen.towers, summary.towers);
    EXPECT_GT(seen.commands, 0U);
}

// A caller persisting a `--record` file has no pre-known script.
// It passes an optional replayRecorder instead.
// bootstrap() must register it, and only the input comes back.
// The level, the waves and the towers are all regenerated.
TEST(RunIntegrationTest, TheReplayRecorderReceivesEveryDispatchedEvent)
{
    NiceMock<MockLogger> logger;
    NiceMock<MockEventSink> eventSink;
    const InputEventCodec codec;
    const Translator translator{antwika::i18n::kDefaultLocale};
    ReplaySource source(script());
    TickEventRecorder recorder;

    const BattleSummary summary =
        antwika::tower_defence::bootstrap(TowerDefenceConfig{
            .logger = logger,
            .eventSink = eventSink,
            .inputSource = source,
            .codec = codec,
            .translator = translator,
            .canvas = kCanvas,
            .campaign = campaignConfig(),
            .maxTicks = kMaxTicks,
            .replayRecorder = recorder});

    // Every dispatched event reaches the recorder, ticks and all.
    // What a saved file keeps is the run's *input*.
    // So that is what is asserted here, with the ticks taken out.
    // Nothing about the level or the mobs was ever an event at all.
    std::vector<TickEvent> supplied;
    for (const TickEvent &event : recorder.getEvents())
    {
        if (event.event.name != antwika::engine::events::kTick)
        {
            supplied.push_back(event);
        }
    }

    EXPECT_EQ(supplied, script());
    EXPECT_EQ(
        recorder.getEvents().size(), supplied.size() + summary.ticks);
}

// A run with no store shows a best of nothing and keeps nothing.
TEST(RunIntegrationTest, ARunWithNoStoreStartsFromNothingAndKeepsIt)
{
    const BattleSummary summary = runOnce();
    EXPECT_EQ(summary.previousBest, HighScore{});
    EXPECT_GT(summary.score, 0U);
    EXPECT_EQ(summary.best.bestScore, summary.score);
    EXPECT_EQ(summary.best.bestLevel, summary.level);
}

TEST(RunIntegrationTest, AFirstRunSetsTheRecordItFinishesOn)
{
    FakeScoreStore store;
    const BattleSummary summary = runOnce(&store);

    EXPECT_EQ(summary.previousBest, HighScore{});
    ASSERT_TRUE(store.written.has_value());
    EXPECT_EQ(store.written->bestScore, summary.score);
    EXPECT_EQ(store.written->bestLevel, summary.level);
}

TEST(RunIntegrationTest, ARunThatFallsShortLeavesTheRecordStanding)
{
    FakeScoreStore store;
    store.kept = HighScore{.bestScore = 100000, .bestLevel = 9};

    const BattleSummary summary = runOnce(&store);
    EXPECT_EQ(summary.previousBest.bestScore, 100000U);
    ASSERT_TRUE(store.written.has_value());
    EXPECT_EQ(*store.written, *store.kept);
}

// A record nobody can read does not take the run with it.
// The run starts from nothing and goes on to write over the file.
TEST(RunIntegrationTest, ARecordThatWillNotReadStartsTheRunFromNothing)
{
    FakeScoreStore store;
    store.refuseToRead = true;

    const BattleSummary summary = runOnce(&store);
    EXPECT_EQ(summary.previousBest, HighScore{});
    ASSERT_TRUE(store.written.has_value());
    EXPECT_EQ(store.written->bestScore, summary.score);
}

// A replay reproduces the run it recorded.
// So it may not read a record that run never saw.
// Nor overwrite one it did not earn.
TEST(RunIntegrationTest, AReplayIsHandedNoStoreAtAll)
{
    FakeScoreStore store;
    EXPECT_TRUE(storeIfLive(store, std::nullopt).has_value());
    EXPECT_FALSE(
        storeIfLive(store, std::optional<std::string>{"demo.json"})
            .has_value());
}

TEST(RunIntegrationTest, TheSummaryLineSaysHowTheCampaignEnded)
{
    const BattleSummary unfinished{
        .phase = CampaignPhase::Fighting,
        .previousBest = {},
        .best = {}};
    const BattleSummary won{
        .phase = CampaignPhase::Won, .previousBest = {}, .best = {}};
    const BattleSummary lost{
        .phase = CampaignPhase::Lost, .previousBest = {}, .best = {}};

    EXPECT_THAT(summaryLine(unfinished), ::testing::HasSubstr("left"));
    EXPECT_THAT(summaryLine(won), ::testing::HasSubstr("cleared"));
    EXPECT_THAT(summaryLine(lost), ::testing::HasSubstr("overrun"));
}

// Lives are what a leak costs, and running out ends the campaign.
TEST(RunIntegrationTest, ACampaignCanBeOverrunInsideOneRun)
{
    const BattleSummary summary = runOnce(nullptr, 1, 0);
    EXPECT_EQ(summary.phase, CampaignPhase::Lost);
    EXPECT_EQ(summary.lives, 0U);
}
