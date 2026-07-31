#include "antwika/companion/Companion.hpp"

#include <memory>
#include <vector>

#include <antwika/engine/Engine.hpp>
#include <antwika/engine/StopSignal.hpp>
#include <antwika/event/EventDispatcher.hpp>
#include <antwika/event/TickedEventDispatcher.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/replay/EngineLoop.hpp>

#include "antwika/companion/PacingSink.hpp"
#include "antwika/companion/PetSink.hpp"
#include "antwika/companion/TapSink.hpp"

namespace antwika::companion
{

    using antwika::engine::Engine;
    using antwika::engine::StopSignal;
    using antwika::event::EventDispatcher;
    using antwika::event::TickedEventDispatcher;
    using antwika::log::Level;
    using antwika::replay::EngineLoop;

    CompanionSummary bootstrap(const CompanionConfig &config)
    {
        ILogger &logger = config.logger;

        Pet pet(config.pet);

        EventDispatcher dispatcher({config.eventSink});

        // The order is the whole wiring.
        // The tap first, answered by the state the last tick ended with.
        // Then the step, which is what sees the meal.
        // Then whatever draws it, so a frame is of the finished tick.
        // Then the wait, which makes the order present-then-wait.
        TapSink tapSink(pet, config.codec);
        PetSink petSink(pet);
        PacingSink pacing(logger, config.sleeper, config.tickInterval);
        StopSignal stopSignal;

        std::vector<std::reference_wrapper<ITickEventSink>> timedSinks{
            tapSink, petSink, stopSignal};

        // Held out here rather than inside the if.
        // The sink has to outlive the reference the dispatcher keeps.
        std::unique_ptr<ITickEventSink> extra;
        if (config.extraSink)
        {
            extra = config.extraSink(pet);
            timedSinks.push_back(*extra);
        }

        timedSinks.push_back(pacing);

        if (config.replayRecorder.has_value())
        {
            timedSinks.push_back(config.replayRecorder->get());
        }

        TickedEventDispatcher tickedDispatcher(dispatcher, timedSinks);
        Engine engine(logger, tickedDispatcher);

        logger.log(Level::Info, "Running Antwika Companion");
        engine.start();

        EngineLoop loop(engine, tickedDispatcher, config.inputSource);
        loop.run(stopSignal, config.maxTicks);

        return CompanionSummary{
            .ticks = pet.ticks(),
            .hunger = pet.hunger(),
            .happiness = pet.happiness(),
            .meals = pet.meals(),
            .disturbances = pet.disturbances(),
            .perished = pet.state() == PetState::Perished};
    }

    void announceHowToStop(ILogger &logger, const bool drawsNothing)
    {
        if (!drawsNothing)
        {
            return;
        }

        logger.log(
            Level::Info,
            "Antwika Companion: this backend has no window to close, so "
            "press Ctrl+C to stop");
    }

    std::string summaryLine(const CompanionSummary &summary)
    {
        const std::string counts =
            std::to_string(summary.ticks) + " ticks, "
            + std::to_string(summary.meals) + " meals, "
            + std::to_string(summary.disturbances)
            + " rude awakenings, happiness "
            + std::to_string(summary.happiness) + ", hunger "
            + std::to_string(summary.hunger);

        if (summary.perished)
        {
            return "The companion perished after " + counts;
        }

        return "The companion is still with us after " + counts;
    }

} // namespace antwika::companion
