#include <chrono>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

#include <antwika/time/SystemSleeper.hpp>
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
    constexpr std::string_view kName = "antwika_gfx_demo";

    constexpr std::chrono::milliseconds kFramePeriod{16};

    constexpr std::optional<std::uint32_t> kUntilWindowClosed = std::nullopt;
}

int main(int argc, char **argv)
{
    ConsoleLogging logging(std::cout, Level::Info);
    auto &logger = logging.logger();

    return runGuarded(
        kName,
        [&logger, argc, argv]
        {
            const auto command =
                antwika::cli::parseCommandLine(argc, argv, {});

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
