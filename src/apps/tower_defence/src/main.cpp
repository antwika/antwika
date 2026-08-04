#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

#include <antwika/app/ConsoleLogging.hpp>
#include <antwika/app/RunRecorded.hpp>
#include <antwika/gfx/SelectedBackend.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/InputPipeline.hpp>
#include <antwika/input/SelectedInputBackend.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/app/WindowInputSource.hpp>
#include <antwika/time/SystemSleeper.hpp>

#include <antwika/app/AssetPath.hpp>
#include "antwika/tower_defence/ConfigFile.hpp"
#include "antwika/tower_defence/BattleScene.hpp"
#include "antwika/tower_defence/FileScoreStore.hpp"
#include "antwika/tower_defence/Messages.hpp"
#include "antwika/tower_defence/RenderSink.hpp"
#include "antwika/tower_defence/TowerDefence.hpp"

using antwika::app::ConsoleLogging;
using antwika::app::RecordedRun;
using antwika::gfx::WindowDesc;
using antwika::input::InputEventCodec;
using antwika::input::InputPipeline;
using antwika::log::Level;
using antwika::replay::ReplaySource;
using antwika::app::WindowInputSource;
using antwika::time::SystemSleeper;
using antwika::tower_defence::BattleScene;
using antwika::tower_defence::BattleSummary;
using antwika::tower_defence::Campaign;
using antwika::tower_defence::FileScoreStore;
using antwika::tower_defence::RenderSink;
using antwika::tower_defence::ScoreOverlay;
using antwika::tower_defence::Translator;

namespace
{
    // A whole number of pixels per cell at the default level size.
    // The score bar's strip comes off the top first.
    constexpr antwika::gfx::Size kWindowSize{
        .width = 960, .height = 720};


    // Where the record waits between one run and the next.
    // Beside the working directory rather than beside the executable.
    // A high score is what somebody's run made, not a shipped asset.
    // So it does not belong in the directory a build writes.
    constexpr std::string_view kScoreFile =
        "tower_defence_highscore.json";

    void run(const RecordedRun &recorded)
    {
        // The numbers the run reads off config.json, once.
        const auto config =
            antwika::tower_defence::loadConfigFileOrDefaults(
                antwika::app::assetPath("config.json"));

        ConsoleLogging logging(std::cout, Level::Info);
        auto &logger = logging.logger();

        const auto backend = antwika::gfx::makeSelectedBackend(logger);
        const auto inputBackend =
            antwika::input::makeSelectedInputBackend(logger);

        logger.log(
            Level::Info,
            "Antwika Tower Defence on backend: "
                + std::string(backend->name()) + ", input: "
                + std::string(inputBackend->name()));

        const auto window = backend->createWindow(WindowDesc{
            .title = "Antwika Tower Defence",
            .size = kWindowSize,
            .resizable = false});

        // Fixed here rather than read from anywhere.
        // The score bar is laid out from translated text.
        // A locale from an environment or a flag is not recorded.
        // Changing the language is this line, as the window size is.
        const Translator translator{antwika::i18n::kDefaultLocale};

        const BattleScene scene;
        SystemSleeper sleeper;
        FileScoreStore store{std::string(kScoreFile)};

        ReplaySource fileSource(
            antwika::app::scriptedEvents(recorded.options.replayPath));

        const InputEventCodec codec;

        // Live input is attached only when there is no replay to run.
        // A replay already holds the input it recorded.
        // Movement is coalesced and idle movement held back.
        // Nothing here follows a free-moving pointer.
        // Only presses build, so what is between them is not recorded.
        InputPipeline input(
            fileSource,
            *inputBackend,
            codec,
            {.readsDevice = !recorded.options.replayPath.has_value(),
             .coalescePointerMotion = true,
             .thinIdleMotion = true});

        WindowInputSource source(input, *backend, window->id());

        const BattleSummary summary =
            antwika::tower_defence::bootstrap({
                .logger = logger,
                .eventSink = recorded.eventSink,
                .inputSource = source,
                .codec = codec,
                .translator = translator,
                .canvas = kWindowSize,
                .campaign =
                    {.lives = config.startingLives,
                     .mobs = config.mobs},
                .scoreStore = antwika::tower_defence::storeIfLive(
                    store, recorded.options.replayPath),
                .replayRecorder = recorded.replayRecorder,
                .extraSink =
                    [&](const Campaign &campaign,
                        const ScoreOverlay &overlay)
                {
                    return std::make_unique<RenderSink>(
                        *window,
                        scene,
                        campaign,
                        overlay,
                        sleeper,
                        std::chrono::milliseconds(
                            config.framePeriodMs),
                        kWindowSize);
                }});

        logger.log(
            Level::Info, antwika::tower_defence::summaryLine(summary));
    }
} // namespace

int main(int argc, char **argv)
{
    return antwika::app::runRecorded(
        argc, argv, "antwika_tower_defence", run);
}
