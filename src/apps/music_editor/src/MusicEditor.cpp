#include "antwika/music_editor/MusicEditor.hpp"

#include <functional>
#include <memory>
#include <vector>

#include <antwika/engine/Engine.hpp>
#include <antwika/engine/StopSignal.hpp>
#include <antwika/event/EventDispatcher.hpp>
#include <antwika/event/TickedEventDispatcher.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/simulation/EngineLoop.hpp>

#include "antwika/music_editor/EditorSink.hpp"
#include "antwika/music_editor/Score.hpp"
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

        std::vector<std::reference_wrapper<ITickEventSink>> timedSinks{
            editor, stopSignal};

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
            .commands = editor.commands().size()};
    }

} // namespace antwika::music_editor
