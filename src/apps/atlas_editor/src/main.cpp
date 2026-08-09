#include <chrono>
#include <iostream>
#include <memory>
#include <string>

#include <antwika/app/FullscreenToggleSource.hpp>
#include <antwika/app/RunRecorded.hpp>
#include <antwika/app/WindowedHost.hpp>
#include <antwika/console/ConsolePicture.hpp>
#include <antwika/console/SnapshotCommands.hpp>
#include <antwika/gfx/SelectedBackend.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/SelectedInputBackend.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/time/SystemSleeper.hpp>
#include <antwika/app/AssetPath.hpp>
#include <antwika/app/TickLimitSource.hpp>

#include "antwika/atlas_editor/ConfigFile.hpp"
#include "antwika/atlas_editor/AtlasEditor.hpp"
#include "antwika/atlas_editor/EditorOptions.hpp"
#include "antwika/atlas_editor/EditorScene.hpp"
#include "antwika/atlas_editor/Messages.hpp"
#include "antwika/atlas_editor/PngAtlasStore.hpp"
#include "antwika/atlas_editor/RenderSink.hpp"

using antwika::app::RecordedRun;
using antwika::app::WindowedHost;
using antwika::app::WindowedSessionDesc;
using antwika::atlas_editor::EditorScene;
using antwika::atlas_editor::EditorState;
using antwika::atlas_editor::EditorSummary;
using antwika::atlas_editor::PngAtlasStore;
using antwika::atlas_editor::RenderSink;
using antwika::app::TickLimitSource;
using antwika::atlas_editor::UiOverlay;
using antwika::log::Level;
using antwika::time::SystemSleeper;

namespace
{
    constexpr antwika::gfx::Size kWindowSize{
        .width = 1280, .height = 720};

    constexpr antwika::input::Key kFullscreenKey =
        antwika::input::Key::F10;

    void run(const RecordedRun &recorded)
    {
        const auto config =
            antwika::atlas_editor::loadConfigFileOrDefaults(
                antwika::app::assetPath("config.json"));

        const auto options = antwika::atlas_editor::editorOptionsFrom(
            recorded.commandLine);

        WindowedHost host(
            std::cout,
            Level::Info,
            {.gfx = antwika::gfx::makeSelectedBackend,
             .input = antwika::input::makeSelectedInputBackend},
            WindowedSessionDesc{
                .name = "Antwika Atlas Editor",
                .windowTitle = "Antwika Atlas Editor",
                .canvas = kWindowSize,
                .resizable = true,
                .mapsPointerToCanvas = true,
                .input = {.stopOnKey = antwika::input::Key::Escape},
                .replayPath = recorded.options.replayPath});

        auto &logger = host.logger();
        auto &session = host.session();

        const EditorScene scene;
        SystemSleeper sleeper;

        antwika::console::ConsolePicture consoleOverlay(session.canvas());

        const antwika::atlas_editor::Translator translator{
            antwika::i18n::kDefaultLocale};

        PngAtlasStore store(options.imagePath, options.outPath);

        antwika::app::FullscreenToggleSource fullscreen(
            session.source(),
            session.window(),
            session.codec(),
            kFullscreenKey);

        TickLimitSource source(fullscreen, options.maxTicks);

        const EditorSummary summary = antwika::atlas_editor::bootstrap({
            .logger = logger,
            .eventSink = recorded.eventSink,
            .inputSource = source,
            .codec = session.codec(),
            .store = store,
            .translator = translator,
            .canvas = kWindowSize,
            .blank = options.sheet,
            .tiles = options.tile,
            .openPath = options.imagePath,
            .announceOpening =
                !recorded.options.replayPath.has_value(),
            .consoleOverlay = consoleOverlay,
            .consoleLoadEnabled =
                antwika::console::consoleLoadPermitted(recorded.options),
            .replayRecorder = recorded.replayRecorder,
            .extraSink =
                [&](const EditorState &state, const UiOverlay &overlay)
            {
                return std::make_unique<RenderSink>(
                    session.window(),
                    scene,
                    state,
                    overlay,
                    consoleOverlay,
                    sleeper,
                    std::chrono::milliseconds(
                        config.framePeriodMs));
            }});

        logger.log(
            Level::Info,
            "Changed " + std::to_string(summary.edits) + " pixels over "
                + std::to_string(summary.ticks) + " ticks, saved "
                + std::to_string(summary.saves) + " times");
    }
}

int main(int argc, char **argv)
{
    return antwika::app::runRecorded(
        argc,
        argv,
        "antwika_atlas_editor",
        run,
        antwika::atlas_editor::editorFlags());
}
