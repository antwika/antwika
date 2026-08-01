#include "antwika/atlas_editor/AtlasEditor.hpp"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include <antwika/engine/Engine.hpp>
#include <antwika/engine/StopSignal.hpp>
#include <antwika/event/EventDispatcher.hpp>
#include <antwika/event/TickedEventDispatcher.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/simulation/EngineLoop.hpp>

#include "antwika/atlas_editor/Canvas.hpp"
#include "antwika/atlas_editor/EditorSink.hpp"

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

    EditorSummary bootstrap(const EditorConfig &config)
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

        std::vector<std::reference_wrapper<ITickEventSink>> timedSinks{
            editor, stopSignal};

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

        TickedEventDispatcher tickedDispatcher(dispatcher, timedSinks);
        Engine engine(logger, tickedDispatcher);

        logger.log(
            antwika::log::Level::Info, "Running Antwika Atlas Editor");
        engine.start();

        EngineLoop loop(engine, tickedDispatcher, config.inputSource);
        loop.run(stopSignal, config.maxTicks);

        return EditorSummary{
            .ticks = state.ticks(),
            .edits = state.edits(),
            .saves = state.saves(),
            .loads = state.loads(),
            .image = state.image().size()};
    }

} // namespace antwika::atlas_editor
