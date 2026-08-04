#include "antwika/tower_defence/TowerDefence.hpp"

#include <memory>
#include <utility>
#include <vector>

#include <antwika/engine/Engine.hpp>
#include <antwika/engine/StopSignal.hpp>
#include <antwika/event/EventDispatcher.hpp>
#include <antwika/event/TickedEventDispatcher.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/simulation/EngineLoop.hpp>

#include "antwika/tower_defence/CampaignSink.hpp"
#include "antwika/tower_defence/ScoreFormatError.hpp"
#include "antwika/tower_defence/ScoreSink.hpp"
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
        // The whole of "what is there to beat".
        // A store nobody gave us is a run that keeps nothing.
        // A file that is not there is a first run, not a failure.
        // A file that will not read is said out loud and stepped over.
        // Refusing to start over an unreadable record is worse.
        // It leaves the game unplayable until a file is deleted.
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
            // The handler runs, and a test drives it.
            // What gcov leaves uncovered is the dispatcher's own edge.
            // That one is taken by an exception of some other type.
            // Only load() throws in here, and only this type.
            // So no input reaches it.
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

        // Once, at the end, rather than every tick.
        // A run ended with Ctrl+C therefore keeps nothing.
        // Unlike a --record run, which appends as it goes.
        void keepRecord(
            const TowerDefenceWiring &config, const HighScore &best)
        {
            if (!config.scoreStore.has_value())
            {
                return;
            }

            config.scoreStore->get().save(best);
        }
    } // namespace

    BattleSummary bootstrap(const TowerDefenceWiring &config)
    {
        ILogger &logger = config.logger;

        // Read before the loop rather than inside it.
        // A file's answer can differ between a run and a replay of it.
        // So the one number that comes from one is fixed here.
        // It is handed in, the way the canvas and the seed are.
        const HighScore previous = openRecord(config);

        Campaign campaign(config.campaign);
        ScoreOverlay overlay(config.canvas);

        EventDispatcher dispatcher({config.eventSink});

        // The order is the whole wiring.
        // Placement first, so a click builds before the tick steps.
        // Then the step, then the bar describing what the step left.
        // Then whatever draws it, so a frame is of the finished tick.
        TowerPlacementSink placement(
            campaign, config.codec, config.canvas);
        CampaignSink campaignSink(campaign);
        ScoreSink scoreSink(
            campaign, overlay, config.translator, previous.bestScore);
        StopSignal stopSignal;

        std::vector<std::reference_wrapper<ITickEventSink>> timedSinks{
            placement, campaignSink, scoreSink, stopSignal};

        // Held out here rather than inside the if.
        // The sink has to outlive the reference the dispatcher keeps.
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
            scoreBarStateOf(campaign, previous.bestScore);
        HighScore reached;
        reached.bestScore = campaign.score();
        reached.bestLevel = ended.level;
        const HighScore best = bestOf(previous, reached);

        // After the loop rather than inside it, and after nothing else.
        // A run that threw its way out of the loop keeps nothing.
        keepRecord(config, best);

        return BattleSummary{
            .score = campaign.score(),
            .lives = campaign.lives(),
            .ticks = campaign.ticks(),
            .towers = campaign.battle().towers().size(),
            .pathLength = campaign.battle().level().path.size(),
            .level = ended.level,
            .wavesReleased = campaign.battle().wavesReleased(),
            .phase = campaign.phase(),
            .previousBest = previous,
            .best = best};
    }

    std::optional<std::reference_wrapper<IScoreStore>> storeIfLive(
        IScoreStore &store, const std::optional<std::string> &replayPath)
    {
        if (replayPath.has_value())
        {
            return std::nullopt;
        }

        return store;
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

} // namespace antwika::tower_defence
