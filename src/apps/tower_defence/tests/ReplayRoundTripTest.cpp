#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <sstream>
#include <vector>

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
#include <antwika/replay/ReplayReader.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/replay/ReplayWriter.hpp>

#include "antwika/tower_defence/Campaign.hpp"
#include "antwika/tower_defence/GridLayout.hpp"
#include "antwika/tower_defence/LevelTile.hpp"
#include "antwika/tower_defence/Messages.hpp"
#include "antwika/tower_defence/ScoreOverlay.hpp"
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
using antwika::replay::ReplayReader;
using antwika::replay::ReplaySource;
using antwika::replay::ReplayWriter;
using antwika::tower_defence::BattleConfig;
using antwika::tower_defence::BattleSummary;
using antwika::tower_defence::Campaign;
using antwika::tower_defence::CampaignConfig;
using antwika::tower_defence::CampaignPhase;
using antwika::tower_defence::Cell;
using antwika::tower_defence::cellRect;
using antwika::tower_defence::layoutFor;
using antwika::tower_defence::LevelPlan;
using antwika::tower_defence::MobKind;
using antwika::tower_defence::ScoreOverlay;
using antwika::tower_defence::Tile;
using antwika::tower_defence::TowerDefenceWiring;
using antwika::tower_defence::Translator;
using antwika::tower_defence::Wave;
using antwika::tower_defence::WaveEntry;
using ::testing::NiceMock;

namespace
{
    constexpr Size kCanvas{.width = 960, .height = 720};
    constexpr std::uint32_t kWidth = 7;
    constexpr std::uint32_t kHeight = 5;
    constexpr antwika::time::Tick kStopAt = 150;
    constexpr antwika::time::Tick kMaxTicks = 200;

    CampaignConfig campaignConfig()
    {
        const Wave wave{
            .entries = {WaveEntry{MobKind::Grunt, 3}},
            .spawnPeriodTicks = 4,
            .gapTicks = 0};
        const LevelPlan plan{
            .level =
                {.width = kWidth, .height = kHeight, .wallSpacing = 3},
            .battle =
                BattleConfig{.towerRangeSquared = 4, .towerDamage = 1},
            .waves = {wave}};

        return CampaignConfig{
            .seed = 21, .lives = 8, .levels = {plan, plan}};
    }

    std::vector<Cell> buildableCells(const std::size_t wanted)
    {
        const Campaign probe(campaignConfig());
        const auto &level = probe.battle().level();

        std::vector<Cell> found;
        for (const Cell &on : level.path)
        {
            const Cell around[] = {
                {.x = on.x, .y = on.y + 1},
                {.x = on.x + 1, .y = on.y},
                {.x = on.x, .y = on.y - 1},
                {.x = on.x - 1, .y = on.y}};

            for (const Cell &cell : around)
            {
                if (found.size() >= wanted || cell.x >= level.width
                    || cell.y >= level.height
                    || level.at(cell) != Tile::Empty)
                {
                    continue;
                }
                found.push_back(cell);
            }
        }
        return found;
    }

    std::vector<TickEvent> script()
    {
        const InputEventCodec codec;
        const auto layout = layoutFor(kCanvas, kWidth, kHeight);

        std::vector<TickEvent> events;
        antwika::time::Tick at = 1;
        for (const Cell &cell : buildableCells(3))
        {
            const auto rect = cellRect(*layout, cell);
            events.push_back(TickEvent{
                .tick = at,
                .event = codec.encode(PointerButtonPressed{
                    .button = MouseButton::Left,
                    .position = {
                        .x = rect.origin.x + 4,
                        .y = rect.origin.y + 4}})});
            ++at;
        }

        events.push_back(TickEvent{
            .tick = kStopAt,
            .event = Event{.name = antwika::engine::events::kStop}});
        return events;
    }

    using Trace = std::vector<std::uint64_t>;

    class FakeCampaignTracer final : public antwika::event::ITickEventSink
    {
    public:
        FakeCampaignTracer(const Campaign &campaign, Trace &trace)
            : campaign(campaign), trace(trace)
        {
        }

        void handle(const TickEvent &event) override
        {
            if (event.event.name != antwika::engine::events::kTick)
            {
                return;
            }

            const auto &battle = campaign.battle();
            trace.push_back(campaign.levelIndex());
            trace.push_back(battle.wavesReleased());
            trace.push_back(battle.mobs().size());
            trace.push_back(battle.towers().size());
            trace.push_back(campaign.score());
            trace.push_back(campaign.lives());
        }

    private:
        const Campaign &campaign;
        Trace &trace;
    };

    BattleSummary runFrom(
        const std::vector<TickEvent> &events,
        Trace &trace,
        TickEventRecorder *recorder)
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> eventSink;
        const InputEventCodec codec;
        const Translator translator{antwika::i18n::kDefaultLocale};
        ReplaySource source(events);

        TowerDefenceWiring config{
            .logger = logger,
            .eventSink = eventSink,
            .inputSource = source,
            .codec = codec,
            .translator = translator,
            .canvas = kCanvas,
            .campaign = campaignConfig(),
            .maxTicks = kMaxTicks,
            .extraSink =
                [&trace](
                    const Campaign &campaign, const ScoreOverlay &)
            { return std::make_unique<FakeCampaignTracer>(campaign, trace); }};

        if (recorder != nullptr)
        {
            config.replayRecorder = *recorder;
        }

        return antwika::tower_defence::bootstrap(config);
    }

    TEST(ReplayRoundTripTest, Replay_ReachesTheSameCampaign)
    {
        TickEventRecorder recorder;
        Trace liveTrace;
        const BattleSummary live =
            runFrom(script(), liveTrace, &recorder);

        std::vector<TickEvent> input;
        for (const TickEvent &event : recorder.getEvents())
        {
            if (event.event.name != antwika::engine::events::kTick)
            {
                input.push_back(event);
            }
        }
        EXPECT_EQ(input, script());

        std::ostringstream out;
        const ReplayWriter writer(kCanvas);
        writer.write(input, out);

        std::istringstream in(out.str());
        const ReplayReader reader;
        Trace replayedTrace;
        const BattleSummary replayed =
            runFrom(reader.read(in), replayedTrace, nullptr);

        EXPECT_EQ(replayedTrace, liveTrace);

        EXPECT_EQ(replayed.score, live.score);
        EXPECT_EQ(replayed.level, live.level);
        EXPECT_EQ(replayed.wavesReleased, live.wavesReleased);
        EXPECT_EQ(replayed.lives, live.lives);
        EXPECT_EQ(replayed.towers, live.towers);
        EXPECT_EQ(replayed.pathLength, live.pathLength);
        EXPECT_EQ(replayed.ticks, live.ticks);
        EXPECT_EQ(replayed.phase, live.phase);

        EXPECT_FALSE(liveTrace.empty());
        EXPECT_GT(live.score, 0U);
        EXPECT_GT(live.level, 1U);
        EXPECT_EQ(live.phase, CampaignPhase::Won);
    }
}
