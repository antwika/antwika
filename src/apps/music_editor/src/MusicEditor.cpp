#include "antwika/music_editor/MusicEditor.hpp"

#include <functional>
#include <memory>
#include <vector>

#include <antwika/console/ConsoleGatedSink.hpp>
#include <antwika/console/ConsolePicture.hpp>
#include <antwika/console/ConsoleScene.hpp>
#include <antwika/console/ConsoleSink.hpp>
#include <antwika/console/ConsoleState.hpp>
#include <antwika/console/IConsoleControls.hpp>
#include <antwika/console/InputFold.hpp>
#include <antwika/console/SnapshotCommands.hpp>
#include <antwika/engine/Engine.hpp>
#include <antwika/engine/StopSignal.hpp>
#include <antwika/event/EventDispatcher.hpp>
#include <antwika/event/TickedEventDispatcher.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/simulation/EngineLoop.hpp>

#include "antwika/music_editor/EditorSink.hpp"
#include "antwika/music_editor/Score.hpp"
#include "antwika/music_editor/SnapshotStore.hpp"
#include "antwika/music_editor/WaveImage.hpp"

namespace antwika::music_editor
{

    using antwika::engine::Engine;
    using antwika::engine::StopSignal;
    using antwika::event::EventDispatcher;
    using antwika::event::TickedEventDispatcher;
    using antwika::simulation::EngineLoop;

    EditorSummary bootstrap(const MusicEditorWiring &config)
    {
        ILogger &logger = config.logger;

        EditorState state = openingState();
        state.scores = config.scores;

        Score score;

        Playback playback(
            score,
            config.mixer,
            config.device,
            config.sleeper,
            config.playback);

        EventDispatcher dispatcher({config.eventSink});

        // The order is the whole wiring.
        // The editor reads this tick's input.
        // It re-reads the lines that changed and advances the sound.
        // Then whatever draws, so a frame is of the finished tick.
        StopSignal stopSignal;

        EditorSink editor(
            state, score, playback, config.codec, config.scene,
            config.canvas,
            // The playback's own two numbers.
            // So the pictures are of the sound this very run makes.
            WaveRenderDesc{
                .rate = config.mixer.format().rate,
                .framesPerCycle = config.playback.framesPerCycle},
            config.clipboard, stopSignal, config.scoresDirectory,
            config.writesScores);

        // The console's own picture, which turns the console on.
        // Absent, no sink is registered and the state stays closed.
        // So the gate below forwards everything, untouched.
        antwika::console::ConsolePicture noConsole;
        const bool hasConsole = config.consoleOverlay.has_value();
        antwika::console::ConsolePicture &consolePicture =
            hasConsole ? config.consoleOverlay->get() : noConsole;

        antwika::console::ConsoleState console;
        const antwika::console::ConsoleScene consoleScene;

        // Grave toggles, Enter executes, the Swedish board types.
        // The board the document's own table also defaults to.
        const antwika::console::FixedConsoleControls consoleControls;

        MusicSnapshotStore snapshotStore(state, score, playback);
        antwika::console::SnapshotCommands consoleCommands(
            snapshotStore,
            config.stateDumpPath,
            config.consoleLoadEnabled);

        antwika::console::InputFold fold(config.codec);
        antwika::console::ConsoleSink consoleSink(
            antwika::console::ConsoleSinkSetup{
                .console = console,
                .input = fold,
                .picture = consolePicture,
                .scene = consoleScene,
                .controls = consoleControls,
                .commands = consoleCommands});

        // The console is on top, so what it stands over it takes.
        // The editor is the one sink here reading a key or a pixel.
        antwika::console::ConsoleGatedSink gatedEditor(
            editor, console, fold);

        std::vector<std::reference_wrapper<ITickEventSink>> timedSinks;

        // The fold first, then the console, ahead of the editor.
        // The order every console-mounting application takes.
        if (hasConsole)
        {
            timedSinks.push_back(fold);
            timedSinks.push_back(consoleSink);
        }

        timedSinks.push_back(gatedEditor);
        timedSinks.push_back(stopSignal);

        // Held out here rather than inside the if.
        // The sink has to outlive the reference the dispatcher keeps.
        std::unique_ptr<ITickEventSink> extra;

        if (config.extraSink)
        {
            extra = config.extraSink(editor);
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
            "Running the antwika music editor");

        engine.start();

        EngineLoop loop(engine, tickedDispatcher, config.inputSource);
        loop.run(stopSignal, config.maxTicks);

        return EditorSummary{
            .notes = playback.started(),
            .played = playback.playedTicks(),
            .reparses = score.reparses(),
            .commands = editor.commands().size(),
            .console = console.history()};
    }

} // namespace antwika::music_editor
