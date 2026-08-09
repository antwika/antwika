#include "antwika/atlas_editor/AtlasEditor.hpp"

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

#include "antwika/atlas_editor/OpeningSheet.hpp"
#include "antwika/atlas_editor/Canvas.hpp"
#include "antwika/atlas_editor/EditorSink.hpp"
#include "antwika/atlas_editor/EditorSnapshotStore.hpp"

namespace antwika::atlas_editor
{

    using antwika::engine::Engine;
    using antwika::engine::StopSignal;
    using antwika::event::EventDispatcher;
    using antwika::event::TickedEventDispatcher;
    using antwika::simulation::EngineLoop;

    namespace
    {
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
    }

    EditorSummary bootstrap(const EditorWiring &config)
    {
        ILogger &logger = config.logger;

        EditorState state(
            openingCanvas(config.store, config.blank),
            config.tiles,
            config.canvas);

        if (config.openPath.has_value())
        {
            const auto described =
                config.store.loadMetaFrom(*config.openPath);

            if (described.has_value())
            {
                state.adoptMeta(*described);
            }
        }

        UiOverlay overlay;

        EventDispatcher dispatcher({config.eventSink});

        StopSignal stopSignal;

        EditorSink editor(
            state,
            overlay,
            config.store,
            config.codec,
            config.translator,
            stopSignal);

        antwika::console::InputFold input(config.codec);
        EditorSnapshotStore snapshotStore(state);

        const antwika::console::ConsoleMountSetup consoleSetup{
            .overlay = config.consoleOverlay,
            .input = input,
            .store = snapshotStore,
            .dumpPath = config.stateDumpPath,
            .loadEnabled = config.consoleLoadEnabled,
            .stop = stopSignal}; // GCOVR_EXCL_LINE
        antwika::console::ConsoleMount consoleMount(consoleSetup);

        antwika::console::ConsoleGatedSink gatedEditor =
            consoleMount.gate(editor);

        std::vector<std::reference_wrapper<ITickEventSink>> timedSinks{
            input};

        if (consoleMount.mounted())
        {
            timedSinks.push_back(consoleMount.sink());
        }

        timedSinks.push_back(gatedEditor);
        timedSinks.push_back(stopSignal);

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

        return EditorSummary{ // GCOVR_EXCL_LINE
            .ticks = state.ticks(),
            .edits = state.edits(),
            .saves = state.saves(),
            .loads = state.loads(),
            .image = state.image().size(),
            .console = consoleMount.state().history()};
    }

}
