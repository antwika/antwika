#include <chrono>
#include <iostream>
#include <memory>
#include <string>

#include <antwika/app/ConsoleLogging.hpp>
#include <antwika/app/RunRecorded.hpp>
#include <antwika/gfx/SelectedBackend.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/InputPipeline.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/SelectedInputBackend.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/simulation/WindowInputSource.hpp>
#include <antwika/time/SystemSleeper.hpp>

#include "antwika/atlas_editor/AtlasEditor.hpp"
#include "antwika/atlas_editor/EditorOptions.hpp"
#include "antwika/atlas_editor/EditorScene.hpp"
#include "antwika/atlas_editor/Messages.hpp"
#include "antwika/atlas_editor/PngAtlasStore.hpp"
#include "antwika/atlas_editor/RenderSink.hpp"
#include <antwika/app/TickLimitSource.hpp>

using antwika::app::ConsoleLogging;
using antwika::app::RecordedRun;
using antwika::atlas_editor::EditorScene;
using antwika::atlas_editor::EditorState;
using antwika::atlas_editor::EditorSummary;
using antwika::atlas_editor::PngAtlasStore;
using antwika::atlas_editor::RenderSink;
using antwika::app::TickLimitSource;
using antwika::atlas_editor::UiOverlay;
using antwika::gfx::WindowDesc;
using antwika::input::InputEventCodec;
using antwika::input::InputPipeline;
using antwika::log::Level;
using antwika::replay::ReplaySource;
using antwika::simulation::WindowInputSource;
using antwika::time::SystemSleeper;

namespace
{
    // Wide enough for the game's 1024 by 320 sheet at zoom one.
    // The toolbar sits above it, with room to zoom into a slot.
    constexpr antwika::gfx::Size kWindowSize{
        .width = 1280, .height = 720};

    constexpr std::chrono::milliseconds kFramePeriod{40};

    void run(const RecordedRun &recorded)
    {
        const auto options = antwika::atlas_editor::editorOptionsFrom(
            recorded.commandLine);

        ConsoleLogging logging(std::cout, Level::Info);
        auto &logger = logging.logger();

        const auto backend = antwika::gfx::makeSelectedBackend(logger);
        const auto inputBackend =
            antwika::input::makeSelectedInputBackend(logger);

        logger.log(
            Level::Info,
            "Antwika Atlas Editor on backend: "
                + std::string(backend->name()) + ", input: "
                + std::string(inputBackend->name()));

        const auto window = backend->createWindow(WindowDesc{
            .title = "Antwika Atlas Editor",
            .size = kWindowSize,
            .resizable = false});

        const EditorScene scene;
        SystemSleeper sleeper;

        // Fixed here, and read from nowhere else.
        // The bar is measured from these words.
        // A press is then resolved against the layout they produce.
        // So a language off the environment would move the buttons.
        // Changing it is this line, exactly as the window size is.
        const antwika::atlas_editor::Translator translator{
            antwika::i18n::kDefaultLocale};

        PngAtlasStore store(options.imagePath, options.outPath);

        ReplaySource fileSource(
            antwika::app::scriptedEvents(recorded.options.replayPath));

        const InputEventCodec codec;

        // Neither thinning decorator is attached.
        // Both absences are deliberate.
        // Coalescing keeps only the last movement of a tick.
        // In a paint tool that is every pixel of a stroke but the last.
        // Gating idle movement would freeze the readout between clicks.
        // The pixel readout and the hover square are simulation state.
        // A replay has to reproduce both from what it recorded.
        // Reading them off the hint channel is exactly what is banned.
        // So a `--record` file grows at the window system's rate.
        // That is the price of the movement being the art.
        InputPipeline input(
            fileSource,
            *inputBackend,
            codec,
            {.readsDevice = !recorded.options.replayPath.has_value(),
             .stopOnKey = antwika::input::Key::Escape});

        WindowInputSource windowed(input, *backend, window->id());

        // The cap ends a session by asking it to stop.
        // EngineLoop's own maxTicks throws when it is reached.
        // Running out of the ticks asked for is not a failure.
        // A `--record` run also has to reach its epilogue to save.
        TickLimitSource source(windowed, options.maxTicks);

        const EditorSummary summary = antwika::atlas_editor::bootstrap({
            .logger = logger,
            .eventSink = recorded.eventSink,
            .inputSource = source,
            .codec = codec,
            .store = store,
            .translator = translator,
            .canvas = kWindowSize,
            .blank = options.sheet,
            .tiles = options.tile,
            .replayRecorder = recorded.replayRecorder,
            .extraSink =
                [&](const EditorState &state, const UiOverlay &overlay)
            {
                return std::make_unique<RenderSink>(
                    *window,
                    scene,
                    state,
                    overlay,
                    sleeper,
                    kFramePeriod);
            }});

        logger.log(
            Level::Info,
            "Changed " + std::to_string(summary.edits) + " pixels over "
                + std::to_string(summary.ticks) + " ticks, saved "
                + std::to_string(summary.saves) + " times");
    }
} // namespace

int main(int argc, char **argv)
{
    return antwika::app::runRecorded(
        argc,
        argv,
        "antwika_atlas_editor",
        run,
        antwika::atlas_editor::editorFlags());
}
