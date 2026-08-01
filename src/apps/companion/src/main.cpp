#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

#include <antwika/app/ConsoleLogging.hpp>
#include <antwika/app/RunRecorded.hpp>
#include <antwika/gfx/SelectedBackend.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/InputPipeline.hpp>
#include <antwika/input/SelectedInputBackend.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/simulation/WindowInputSource.hpp>
#include <antwika/time/SystemSleeper.hpp>

#include "antwika/companion/Companion.hpp"
#include "antwika/companion/FilePetStore.hpp"
#include "antwika/companion/PetScene.hpp"
#include "antwika/companion/RenderSink.hpp"

using antwika::app::ConsoleLogging;
using antwika::app::RecordedRun;
using antwika::companion::CompanionSummary;
using antwika::companion::FilePetStore;
using antwika::companion::Lineage;
using antwika::companion::Pet;
using antwika::companion::PetScene;
using antwika::companion::RenderSink;
using antwika::gfx::WindowDesc;
using antwika::input::InputEventCodec;
using antwika::input::InputPipeline;
using antwika::log::Level;
using antwika::replay::ReplaySource;
using antwika::simulation::WindowInputSource;
using antwika::time::SystemSleeper;

namespace
{
    // A companion lives in the corner of a desktop, not in front of one.
    // Everything it draws is laid out on kSceneUnits whole units a side.
    // So the size is that many units rather than a dividing pixel count.
    constexpr std::uint32_t kPixelsPerUnit = 8;

    constexpr antwika::gfx::Size kWindowSize{
        .width = kPixelsPerUnit * antwika::companion::kSceneUnits,
        .height = kPixelsPerUnit * antwika::companion::kSceneUnits};

    // The backend that draws nothing.
    // A build using it has nothing to watch and nothing to close.
    constexpr std::string_view kHeadlessBackendName = "null";

    // Where the companion waits between one session and the next.
    // Beside the working directory rather than beside the executable.
    // A companion is what somebody's session made, not a shipped asset.
    // So it does not belong in the directory a build writes.
    constexpr std::string_view kCompanionFile = "companion.json";

    void run(const RecordedRun &recorded)
    {
        ConsoleLogging logging(std::cout, Level::Info);
        auto &logger = logging.logger();

        const auto backend = antwika::gfx::makeSelectedBackend(logger);
        const auto inputBackend =
            antwika::input::makeSelectedInputBackend(logger);
        const bool drawsNothing = backend->name() == kHeadlessBackendName;

        logger.log(
            Level::Info,
            "Antwika Companion on backend: "
                + std::string(backend->name()) + ", input: "
                + std::string(inputBackend->name()));
        antwika::companion::announceHowToStop(logger, drawsNothing);

        const auto window = backend->createWindow(WindowDesc{
            .title = "Antwika Companion",
            .size = kWindowSize,
            .resizable = false});

        const PetScene scene;
        SystemSleeper sleeper;
        FilePetStore store{std::string(kCompanionFile)};

        ReplaySource fileSource(
            antwika::app::scriptedEvents(recorded.options.replayPath));

        const InputEventCodec codec;

        // Live input is attached only when there is no replay to run.
        // A replay already holds the input it recorded.
        // Movement is coalesced and idle movement is held back.
        // A press is the only input this application has.
        // Where the pointer went between two of them is unreadable here.
        InputPipeline input(
            fileSource,
            *inputBackend,
            codec,
            {.readsDevice = !recorded.options.replayPath.has_value(),
             .coalescePointerMotion = true,
             .thinIdleMotion = true});

        WindowInputSource source(input, *backend, window->id());

        const CompanionSummary summary =
            antwika::companion::bootstrap({
                .logger = logger,
                .eventSink = recorded.eventSink,
                .inputSource = source,
                .codec = codec,
                .sleeper = sleeper,
                .canvas = kWindowSize,
                .store = antwika::companion::storeIfLive(
                    store, recorded.options.replayPath),
                .replayRecorder = recorded.replayRecorder,
                .extraSink =
                    [&](const Pet &pet, const Lineage &lineage)
                {
                    return std::make_unique<RenderSink>(
                        *window, scene, pet, lineage, kWindowSize);
                }});

        logger.log(
            Level::Info, antwika::companion::summaryLine(summary));
    }
} // namespace

int main(int argc, char **argv)
{
    return antwika::app::runRecorded(argc, argv, "antwika_companion", run);
}
