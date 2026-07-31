#include "antwika/tower_defence/TowerDefence.hpp"

#include <memory>
#include <vector>

#include <antwika/engine/Engine.hpp>
#include <antwika/engine/StopSignal.hpp>
#include <antwika/event/EventDispatcher.hpp>
#include <antwika/event/TickedEventDispatcher.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/replay/EngineLoop.hpp>

#include "antwika/tower_defence/BattleSink.hpp"
#include "antwika/tower_defence/ScoreSink.hpp"
#include "antwika/tower_defence/TowerPlacementSink.hpp"

namespace antwika::tower_defence
{

    using antwika::engine::Engine;
    using antwika::engine::StopSignal;
    using antwika::event::EventDispatcher;
    using antwika::event::TickedEventDispatcher;

    using antwika::replay::EngineLoop;

    BattleSummary bootstrap(const TowerDefenceConfig &config)
    {
        ILogger &logger = config.logger;

        Battle battle(generateLevel(config.level), config.battle);
        ScoreOverlay overlay(config.canvas);

        EventDispatcher dispatcher({config.eventSink});

        // The order is the whole wiring.
        // Placement first, so a click builds before the tick steps.
        // Then the step, then the bar describing what the step left.
        // Then whatever draws it, so a frame is of the finished tick.
        TowerPlacementSink placement(battle, config.codec, config.canvas);
        BattleSink battleSink(battle);
        ScoreSink scoreSink(battle, overlay);
        StopSignal stopSignal;

        std::vector<std::reference_wrapper<ITickEventSink>> timedSinks{
            placement, battleSink, scoreSink, stopSignal};

        // Held out here rather than inside the if.
        // The sink has to outlive the reference the dispatcher keeps.
        std::unique_ptr<ITickEventSink> extra;
        if (config.extraSink)
        {
            extra = config.extraSink(battle, overlay);
            timedSinks.push_back(*extra);
        }

        if (config.replayRecorder.has_value())
        {
            timedSinks.push_back(config.replayRecorder->get());
        }

        TickedEventDispatcher tickedDispatcher(dispatcher, timedSinks);
        Engine engine(logger, tickedDispatcher);

        logger.log(
            antwika::log::Level::Info,
            "Running Antwika Tower Defence");
        engine.start();

        EngineLoop loop(engine, tickedDispatcher, config.inputSource);
        loop.run(stopSignal, config.maxTicks);

        return BattleSummary{
            .score = battle.score(),
            .leaks = battle.leaks(),
            .ticks = battle.ticks(),
            .towers = battle.towers().size(),
            .pathLength = battle.level().path.size()};
    }

} // namespace antwika::tower_defence
