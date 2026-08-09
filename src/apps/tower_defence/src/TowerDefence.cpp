#include "antwika/tower_defence/TowerDefence.hpp"

#include <memory>
#include <utility>
#include <vector>

#include <antwika/app/StoreIfLive.hpp>
#include <antwika/engine/Engine.hpp>
#include <antwika/engine/StopSignal.hpp>
#include <antwika/event/EventDispatcher.hpp>
#include <antwika/event/TickedEventDispatcher.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/simulation/EngineLoop.hpp>

#include <antwika/console/ConsoleGatedSink.hpp>
#include <antwika/console/ConsoleMount.hpp>
#include <antwika/console/InputFold.hpp>

#include "antwika/tower_defence/CampaignSink.hpp"
#include "antwika/tower_defence/ScoreFormatError.hpp"
#include "antwika/tower_defence/ScoreSink.hpp"
#include "antwika/tower_defence/TowerDefenceSnapshotStore.hpp"
#include "antwika/tower_defence/TowerPlacementSink.hpp"

namespace antwika::tower_defence
{

    using antwika::engine::Engine;
    using antwika::engine::StopSignal;
    using antwika::event::EventDispatcher;
    using antwika::event::TickedEventDispatcher;
    using antwika::simulation::EngineLoop;

    namespace
    {
        HighScore openRecord(const TowerDefenceWiring &config)
        {
            if (!config.scoreStore.has_value())
            {
                return HighScore{};
            }

            try
            {
                const auto kept = config.scoreStore->get().load();
                if (!kept.has_value())
                {
                    config.logger.log(
                        antwika::log::Level::Info,
                        "No high score yet, so this run sets it");
                    return HighScore{};
                }

                return *kept;
            }
            catch (const ScoreFormatError &error) // GCOVR_EXCL_LINE
            {
                config.logger.log(
                    antwika::log::Level::Warning,
                    std::string("The high score could not be read, so "
                                "this run starts from nothing: ")
                        + error.what());
                return HighScore{};
            }
        }

        void keepRecord(
            const TowerDefenceWiring &config, const HighScore &best)
        {
            if (!config.scoreStore.has_value())
            {
                return;
            }

            config.scoreStore->get().save(best);
        }
    }

    BattleSummary bootstrap(const TowerDefenceWiring &config)
    {
        ILogger &logger = config.logger;

        const HighScore previous = openRecord(config);

        std::uint64_t bestBaseline = previous.bestScore;

        Campaign campaign(config.campaign);
        ScoreOverlay overlay(config.canvas);

        EventDispatcher dispatcher({config.eventSink});

        antwika::console::InputFold input(config.codec);
        TowerPlacementSink placement(
            campaign, config.codec, config.canvas);
        CampaignSink campaignSink(campaign);
        ScoreSink scoreSink(
            campaign, overlay, config.translator, bestBaseline);
        StopSignal stopSignal;

        TowerDefenceSnapshotStore snapshotStore(campaign, bestBaseline);

        const antwika::console::ConsoleMountSetup consoleSetup{
            .overlay = config.consoleOverlay,
            .input = input,
            .store = snapshotStore,
            .dumpPath = config.stateDumpPath,
            .loadEnabled = config.consoleLoadEnabled,
            .stop = stopSignal}; // GCOVR_EXCL_LINE
        antwika::console::ConsoleMount consoleMount(consoleSetup);

        antwika::console::ConsoleGatedSink gatedPlacement =
            consoleMount.gate(placement);

        std::vector<std::reference_wrapper<ITickEventSink>> timedSinks{
            input};

        if (consoleMount.mounted())
        {
            timedSinks.push_back(consoleMount.sink());
        }

        timedSinks.push_back(gatedPlacement);
        timedSinks.push_back(campaignSink);
        timedSinks.push_back(scoreSink);
        timedSinks.push_back(stopSignal);

        std::unique_ptr<ITickEventSink> extra;
        if (config.extraSink)
        {
            extra = config.extraSink(campaign, overlay);
            timedSinks.push_back(*extra);
        }

        if (config.replayRecorder.has_value())
        {
            timedSinks.push_back(config.replayRecorder->get());
        }

        TickedEventDispatcher tickedDispatcher(dispatcher, timedSinks);
        Engine engine(logger, tickedDispatcher);

        logger.log(
            antwika::log::Level::Info, "Running Antwika Tower Defence");
        engine.start();

        EngineLoop loop(engine, tickedDispatcher, config.inputSource);
        loop.run(stopSignal, config.maxTicks);

        const ScoreBarState ended =
            scoreBarStateOf(campaign, bestBaseline);
        HighScore reached;
        reached.bestScore = campaign.score();
        reached.bestLevel = ended.level;
        const HighScore best = bestOf(previous, reached);

        keepRecord(config, best);

        return BattleSummary{ // GCOVR_EXCL_LINE
            .score = campaign.score(),
            .lives = campaign.lives(),
            .ticks = campaign.ticks(),
            .towers = campaign.battle().towers().size(),
            .pathLength = campaign.battle().level().path.size(),
            .level = ended.level,
            .wavesReleased = campaign.battle().wavesReleased(),
            .phase = campaign.phase(),
            .previousBest = previous,
            .best = best,
            .console = consoleMount.state().history()};
    } // GCOVR_EXCL_LINE

    std::optional<std::reference_wrapper<IScoreStore>> storeIfLive(
        IScoreStore &store,
        const std::optional<std::string> &replayPath)
    {
        return antwika::app::storeIfLive(store, replayPath);
    }

    std::string summaryLine(const BattleSummary &summary)
    {
        const std::string counts =
            std::to_string(summary.score) + " over "
            + std::to_string(summary.ticks) + " ticks, reaching level "
            + std::to_string(summary.level) + " with "
            + std::to_string(summary.lives) + " lives left (best so far "
            + std::to_string(summary.best.bestScore) + " on level "
            + std::to_string(summary.best.bestLevel) + ")";

        if (summary.phase == CampaignPhase::Won)
        {
            return "The campaign was cleared, scoring " + counts;
        }
        if (summary.phase == CampaignPhase::Lost)
        {
            return "The campaign was overrun, scoring " + counts;
        }

        return "The campaign was left unfinished, scoring " + counts;
    }

}
