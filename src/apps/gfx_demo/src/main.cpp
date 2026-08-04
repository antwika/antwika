#include <antwika/time/SystemSleeper.hpp>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

#include <antwika/app/AssetPath.hpp>
#include <antwika/app/ConsoleLogging.hpp>
#include <antwika/app/PngFile.hpp>
#include <antwika/app/RunGuarded.hpp>
#include <antwika/cli/CommandLine.hpp>
#include <antwika/gfx/SelectedBackend.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/input/SelectedInputBackend.hpp>
#include <antwika/log/Level.hpp>

#include "antwika/gfx_demo/DemoLoop.hpp"
#include "antwika/gfx_demo/DemoScene.hpp"

using antwika::app::assetPath;
using antwika::app::ConsoleLogging;
using antwika::app::readPngFile;
using antwika::app::runGuarded;
using antwika::gfx::WindowDesc;
using antwika::gfx_demo::DemoLoop;
using antwika::gfx_demo::DemoScene;
using antwika::log::Level;

namespace
{
    // At namespace scope for the reason sound_demo's main.cpp gives.
    // A local would be odr-used by the lambda below.
    constexpr std::string_view kName = "antwika_gfx_demo";

    // The demo is something to look at, so it draws until it is closed.
    // The null backend reports no close, so that build never finishes.
    // Sixty a second, which is what a window is refreshed at.
    // A demo with nothing to wait for would otherwise spin a core.
    constexpr std::chrono::milliseconds kFramePeriod{16};

    constexpr std::optional<std::uint32_t> kUntilWindowClosed = std::nullopt;
} // namespace

int main(int argc, char **argv)
{
    ConsoleLogging logging(std::cout, Level::Info);
    auto &logger = logging.logger();

    return runGuarded(
        kName,
        [&logger, argc, argv]
        {
            // This demo takes no flags of its own.
            // That is a thing to say rather than a reason to read none.
            // An empty table still answers --help and refuses a typo.
            // A refused flag is a failed run rather than a crash.
            // So it is parsed inside the guard, as sound_demo's is.
            const auto command =
                antwika::cli::parseCommandLine(argc, argv, {});

            // --help is a question, not a run.
            // Answering it opens no window.
            if (command.has(antwika::cli::kHelpFlag))
            {
                std::cout << antwika::cli::helpText(kName, {});
                return;
            }

            const auto backend = antwika::gfx::makeSelectedBackend(logger);
            const auto inputBackend =
                antwika::input::makeSelectedInputBackend(logger);

            logger.log(
                Level::Info,
                "Antwika gfx demo on backends: "
                    + std::string(backend->name()) + " / "
                    + std::string(inputBackend->name()));

            const auto logo = readPngFile(
                assetPath("antwika.png"), "antwika_gfx_demo");

            const DemoScene scene;
            antwika::time::SystemSleeper sleeper;
            DemoLoop loop(
                *backend, *inputBackend, scene, sleeper, kFramePeriod);

            loop.run(
                WindowDesc{
                    .title = "Antwika gfx demo",
                    .size = {.width = 800, .height = 600}},
                logo,
                kUntilWindowClosed);
        });
}
