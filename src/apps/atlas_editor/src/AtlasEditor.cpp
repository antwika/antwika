#include "antwika/atlas_editor/AtlasEditor.hpp"

#include "antwika/atlas_editor/OpeningSheet.hpp"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include <antwika/console/ConsoleGatedSink.hpp>
#include <antwika/console/ConsoleMount.hpp>
#include <antwika/console/InputFold.hpp>
#include <antwika/engine/Engine.hpp>
#include <antwika/engine/StopSignal.hpp>
#include <antwika/event/EventDispatcher.hpp>
#include <antwika/event/TickedEventDispatcher.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/simulation/EngineLoop.hpp>

#include "antwika/atlas_editor/Canvas.hpp"
#include "antwika/atlas_editor/EditorSink.hpp"
#include "antwika/atlas_editor/SnapshotStore.hpp"

namespace antwika::atlas_editor
{

    using antwika::engine::Engine;
    using antwika::engine::StopSignal;
    using antwika::event::EventDispatcher;
    using antwika::event::TickedEventDispatcher;
    using antwika::simulation::EngineLoop;

    namespace
    {
        // Where "open a file, or start blank" is decided.
        // It lives here rather than in a main().
        // A main() is branchless by rule.
        // It is also the one file coverage does not measure.
        [[nodiscard]] Canvas openingCanvas(
            IAtlasStore &store, const Size blank)
        {
            auto loaded = store.load();

            if (loaded.has_value())
            {
                return Canvas(std::move(*loaded));
            }

            return Canvas::blank(blank);
        }
    } // namespace

    EditorSummary bootstrap(const EditorWiring &config)
    {
        ILogger &logger = config.logger;

        EditorState state(
            openingCanvas(config.store, config.blank),
            config.tiles,
            config.canvas);
        UiOverlay overlay;

        EventDispatcher dispatcher({config.eventSink});

        // The order is the whole wiring.
        // The editor first, so a click has been acted on.
        // Then whatever draws it.
        // A frame is then of the tick that has just finished.
        EditorSink editor(
            state,
            overlay,
            config.store,
            config.codec,
            config.translator);
        StopSignal stopSignal;

        antwika::console::InputFold input(config.codec);
        EditorSnapshotStore snapshotStore(state);

        // The overlay is the console's own picture, which turns it on.
        // Absent, no sink is registered and the state stays closed.
        // So the gate below forwards everything, untouched.
        // This application rebinds nothing.
        // No controls are named, so the shipped constants stand.
        // Grave, Enter and the Swedish board.
        // Grave conflicts with nothing here.
        // F10 and Escape stay reserved upstream, untouched.
        // A named setup rather than a temporary in the call.
        // gcov parks a temporary's unwind code on its head line.
        const antwika::console::ConsoleMountSetup consoleSetup{
            .overlay = config.consoleOverlay,
            .input = input,
            .store = snapshotStore,
            .dumpPath = config.stateDumpPath,
            .loadEnabled = config.consoleLoadEnabled};
        antwika::console::ConsoleMount consoleMount(consoleSetup);

        // The console is on top, so what it stands over it takes.
        // EditorSink is the one sink here reading a key or a pixel.
        // Its own private fold is left alone.
        // The gate simply stops swallowed events reaching it.
        antwika::console::ConsoleGatedSink gatedEditor =
            consoleMount.gate(editor);

        // The fold is first, ahead of everything that reads it.
        // ConsoleSink is next, ahead of everything it gates.
        // A press has to be the console's before the editor may ask.
        std::vector<std::reference_wrapper<ITickEventSink>> timedSinks{
            input};

        // Registered only when there is somewhere to put the picture.
        // "No console" then means no console, not an invisible one.
        if (consoleMount.mounted())
        {
            timedSinks.push_back(consoleMount.sink());
        }

        timedSinks.push_back(gatedEditor);
        timedSinks.push_back(stopSignal);

        // Held out here rather than inside the if.
        // The sink has to outlive the reference the dispatcher keeps.
        std::unique_ptr<ITickEventSink> extra;
        if (config.extraSink)
        {
            extra = config.extraSink(state, overlay);
            timedSinks.push_back(*extra);
        }

        if (config.replayRecorder.has_value())
        {
            timedSinks.push_back(config.replayRecorder->get());
        }

        // Ahead of the recorder, so a recording carries its sheet.
        // A replay run passes no announcement; its recording has one.
        std::optional<antwika::event::Event> announcement;

        if (config.announceOpening)
        {
            announcement = openingSheetEvent(state.image());
        }

        OpeningSheetSource announced(
            config.inputSource, std::move(announcement));

        TickedEventDispatcher tickedDispatcher(dispatcher, timedSinks);
        Engine engine(logger, tickedDispatcher);

        logger.log(
            antwika::log::Level::Info, "Running Antwika Atlas Editor");
        engine.start();

        EngineLoop loop(engine, tickedDispatcher, announced);
        loop.run(stopSignal, config.maxTicks);

        // The branch left on the excluded line is the allocator's.
        // The throw edge of copying the history into the summary.
        return EditorSummary{ // GCOVR_EXCL_LINE
            .ticks = state.ticks(),
            .edits = state.edits(),
            .saves = state.saves(),
            .loads = state.loads(),
            .image = state.image().size(),
            .console = consoleMount.state().history()};
    }

} // namespace antwika::atlas_editor
