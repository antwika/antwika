#include "antwika/companion/Companion.hpp"

#include <memory>
#include <vector>

#include <antwika/engine/Engine.hpp>
#include <antwika/engine/StopSignal.hpp>
#include <antwika/event/EventDispatcher.hpp>
#include <antwika/event/TickedEventDispatcher.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/simulation/EngineLoop.hpp>

#include "antwika/companion/CompanionMemory.hpp"
#include "antwika/companion/PacingSink.hpp"
#include "antwika/companion/PetSink.hpp"
#include "antwika/companion/PropSink.hpp"
#include "antwika/companion/ReviveSink.hpp"
#include "antwika/companion/SaveFormatError.hpp"

namespace antwika::companion
{

    using antwika::engine::Engine;
    using antwika::engine::StopSignal;
    using antwika::event::EventDispatcher;
    using antwika::event::TickedEventDispatcher;
    using antwika::log::Level;
    using antwika::simulation::EngineLoop;

    namespace
    {
        // The companion, and the record behind it.
        // The two have different lifetimes.
        // A revival replaces the first and leaves the second standing.
        struct Session
        {
            Pet pet;
            Lineage lineage;
        };

        // The whole of "carry on from where the last session left off".
        // A store nobody gave us is a session that keeps nothing.
        // A file that is not there is a first run, not a failure.
        // A file that will not read is said out loud and stepped over.
        // A companion nobody can read is a companion that is gone.
        // And refusing to start would leave the app unusable.
        // Until somebody went and deleted a file by hand.
        Session openSession(const CompanionConfig &config)
        {
            if (!config.store.has_value())
            {
                return Session{Pet(config.pet), Lineage()};
            }

            try
            {
                const auto memory = config.store->get().load();

                if (!memory.has_value())
                {
                    config.logger.log(
                        Level::Info,
                        "No previous companion, so this is a new one");
                    return Session{Pet(config.pet), Lineage()};
                }

                config.logger.log(
                    Level::Info,
                    "Carrying on with the companion from last time");
                return Session{
                    Pet(config.pet, memory->pet),
                    Lineage(memory->lineage)};
            }
            catch (const SaveFormatError &error)
            {
                config.logger.log(
                    Level::Warning,
                    std::string("The saved companion could not be read, "
                                "so this is a new one: ")
                        + error.what());
                return Session{Pet(config.pet), Lineage()};
            }
        }

        // Once, at the end, rather than every tick.
        // A run ended with Ctrl+C therefore keeps nothing.
        // Unlike a --record run, which appends as it goes.
        // The age is offered to the record on the way out.
        // So a companion nobody replaced still sets the mark it earned.
        void keepSession(
            const CompanionConfig &config,
            const Pet &pet,
            Lineage &lineage)
        {
            if (!config.store.has_value())
            {
                return;
            }

            lineage.record(pet.ticks());
            config.store->get().save(CompanionMemory{
                .pet = pet.remember(), .lineage = lineage.remember()});
        }
    } // namespace

    CompanionSummary bootstrap(const CompanionConfig &config)
    {
        ILogger &logger = config.logger;

        Session session = openSession(config);
        Pet &pet = session.pet;
        Lineage &lineage = session.lineage;

        EventDispatcher dispatcher({config.eventSink});

        // The order is the whole wiring.
        // The press first, answered by the state the last tick left.
        // Then the button, which one press may not also be.
        // A press on a perished companion means nothing to PropSink.
        // The other order would start a companion and then prod it.
        // Then the step, which is what sees the meal.
        // Then whatever draws it, so a frame is of the finished tick.
        // Then the wait, which makes the order present-then-wait.
        PropSink propSink(pet, config.codec, config.canvas);
        ReviveSink reviveSink(pet, lineage, config.codec, config.canvas);
        PetSink petSink(pet);
        PacingSink pacing(logger, config.sleeper, config.tickInterval);
        StopSignal stopSignal;

        std::vector<std::reference_wrapper<ITickEventSink>> timedSinks{
            propSink, reviveSink, petSink, stopSignal};

        // Held out here rather than inside the if.
        // The sink has to outlive the reference the dispatcher keeps.
        std::unique_ptr<ITickEventSink> extra;
        if (config.extraSink)
        {
            extra = config.extraSink(pet, lineage);
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

        // After the loop rather than inside it, and after nothing else.
        // A session that threw its way out of the loop keeps nothing.
        // What it would keep is whatever state the failure left.
        keepSession(config, pet, lineage);

        return CompanionSummary{
            .ticks = pet.ticks(),
            .day = pet.day(),
            .hunger = pet.hunger(),
            .fun = pet.fun(),
            .happiness = pet.happiness(),
            .energy = pet.energy(),
            .energyCeiling = pet.energyCeiling(),
            .meals = pet.meals(),
            .plays = pet.plays(),
            .disturbances = pet.disturbances(),
            .pesters = pet.pesters(),
            .collapses = pet.collapses(),
            .generation = lineage.generation(),
            .bestTicks = lineage.bestTicks(),
            .perished = pet.state() == PetState::Perished};
    }

    std::optional<std::reference_wrapper<IPetStore>> storeIfLive(
        IPetStore &store, const std::optional<std::string> &replayPath)
    {
        if (replayPath.has_value())
        {
            return std::nullopt;
        }

        return store;
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
            std::to_string(summary.ticks) + " ticks over "
            + std::to_string(summary.day) + " days, "
            + std::to_string(summary.meals) + " meals, "
            + std::to_string(summary.plays) + " games, "
            + std::to_string(summary.disturbances) + " rude awakenings, "
            + std::to_string(summary.pesters) + " unwanted attentions, "
            + std::to_string(summary.collapses) + " collapses, energy "
            + std::to_string(summary.energy) + "/"
            + std::to_string(summary.energyCeiling) + " (best so far "
            + std::to_string(summary.bestTicks)
            + " ticks, companion number "
            + std::to_string(summary.generation) + ")";

        if (summary.perished)
        {
            return "The companion perished after " + counts;
        }

        return "The companion is still with us after " + counts;
    }

} // namespace antwika::companion
