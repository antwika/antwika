#include "antwika/companion/Companion.hpp"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include <antwika/app/StoreIfLive.hpp>
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
#include "antwika/companion/RestoreSink.hpp"
#include "antwika/companion/RestoreSource.hpp"
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
        // The whole of "carry on from where the last session left off".
        // A store nobody gave us is a session that keeps nothing.
        // A file that is not there is a first run, not a failure.
        // A file that will not read is said out loud and stepped over.
        // A companion nobody can read is a companion that is gone.
        // And refusing to start would leave the app unusable.
        // Until somebody went and deleted a file by hand.
        //
        // What comes back is the memory, not a companion built from it.
        // A session is restored through the event stream instead.
        // Never through a constructor -- see RestoreSource.
        std::optional<CompanionMemory> openSession(
            const CompanionWiring &config)
        {
            if (!config.store.has_value())
            {
                return std::nullopt;
            }

            try
            {
                const auto memory = config.store->get().load();

                if (!memory.has_value())
                {
                    config.logger.log(
                        Level::Info,
                        "No previous companion, so this is a new one");
                    return std::nullopt;
                }

                // Built and thrown away, deliberately.
                // Pet alone says whether a memory is a possible one.
                // Which is nobody else's question to answer.
                // It is asked here rather than in the sink.
                // A file that will not read must not take a tick with it.
                (void)Pet(config.pet, memory->pet);
                (void)Lineage(memory->lineage);

                config.logger.log(
                    Level::Info,
                    "Carrying on with the companion from last time");
                return memory;
            }
            catch (const SaveFormatError &error)
            {
                config.logger.log(
                    Level::Warning,
                    std::string("The saved companion could not be read, "
                                "so this is a new one: ")
                        + error.what());
                return std::nullopt;
            }
        }

        // Once, at the end, rather than every tick.
        // A run ended with Ctrl+C therefore keeps nothing.
        // Unlike a --record run, which appends as it goes.
        void keepSession(
            const CompanionWiring &config,
            const Pet &pet,
            const Lineage &lineage)
        {
            if (!config.store.has_value())
            {
                return;
            }

            config.store->get().save(CompanionMemory{
                .pet = pet.remember(), .lineage = lineage.remember()});
        }
    } // namespace

    CompanionSummary bootstrap(const CompanionWiring &config)
    {
        ILogger &logger = config.logger;

        std::optional<CompanionMemory> remembered = openSession(config);

        // Always new, and restored through the stream if at all.
        // A live run and the replay of it then take one road in.
        Pet pet(config.pet);
        Lineage lineage;

        EventDispatcher dispatcher({config.eventSink});

        // The order is the whole wiring.
        // The companion first.
        // A press on the tick it arrives on lands on the restored one.
        // Then the press, answered by the state the last tick left.
        // Then the button, which one press may not also be.
        // A press on a perished companion means nothing to PropSink.
        // The other order would start a companion and then prod it.
        // Then the step, which is what sees the meal.
        // Then whatever draws it, so a frame is of the finished tick.
        // Then the wait, which makes the order present-then-wait.
        RestoreSink restoreSink(pet, lineage);
        PropSink propSink(pet, config.codec, config.canvas);
        ReviveSink reviveSink(pet, lineage, config.codec, config.canvas);
        PetSink petSink(pet);
        PacingSink pacing(logger, config.sleeper, config.tickInterval);
        StopSignal stopSignal;

        std::vector<std::reference_wrapper<ITickEventSink>> timedSinks{
            restoreSink, propSink, reviveSink, petSink, stopSignal};

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

        // Upstream of the recorder, which is the whole arrangement.
        // A `--record` run writes the companion it was played on.
        // So replaying that file starts on the same animal.
        RestoreSource restoreSource(
            config.inputSource, std::move(remembered));

        EngineLoop loop(engine, tickedDispatcher, restoreSource);
        loop.run(stopSignal, config.maxTicks);

        // The age is offered to the record on the way out.
        // So a companion nobody replaced still sets the mark it earned.
        // Whether there is a file behind the session or not.
        // What a session ends on may not depend on there being one.
        // A replay has no store by design, and would report another best.
        lineage.record(pet.ticks());

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
        IPetStore &store,
        const std::optional<std::string> &replayPath)
    {
        return antwika::app::storeIfLive(store, replayPath);
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
