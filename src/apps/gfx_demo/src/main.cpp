#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <optional>
#include <string>

#include <antwika/app/ConsoleLogging.hpp>
#include <antwika/app/PngFile.hpp>
#include <antwika/gfx/SelectedBackend.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/input/SelectedInputBackend.hpp>
#include <antwika/log/Level.hpp>

#include "antwika/gfx_demo/DemoLoop.hpp"
#include "antwika/gfx_demo/DemoScene.hpp"

using antwika::app::ConsoleLogging;
using antwika::app::readPngFile;
using antwika::gfx::WindowDesc;
using antwika::gfx_demo::DemoLoop;
using antwika::gfx_demo::DemoScene;
using antwika::log::Level;

namespace
{
    // The demo is something to look at, so it draws until it is closed.
    // The null backend reports no close, so that build never finishes.
    constexpr std::optional<std::uint32_t> kUntilWindowClosed = std::nullopt;
} // namespace

int main()
{
    ConsoleLogging logging(std::cout, Level::Info);
    auto &logger = logging.logger();

    // Catching is what makes the run's resources unwind at all.
    // An uncaught exception may call std::terminate without unwinding.
    // A window and a texture are what would be left behind here.
    int exitCode = EXIT_SUCCESS;
    try
    {
        const auto backend = antwika::gfx::makeSelectedBackend(logger);
        const auto inputBackend =
            antwika::input::makeSelectedInputBackend(logger);

        logger.log(
            Level::Info,
            "Antwika gfx demo on backends: " + std::string(backend->name())
                + " / " + std::string(inputBackend->name()));

        const auto logo = readPngFile(
            ANTWIKA_GFX_DEMO_TEXTURE_PATH, "antwika_gfx_demo");

        const DemoScene scene;
        DemoLoop loop(*backend, *inputBackend, scene);

        loop.run(
            WindowDesc{
                .title = "Antwika gfx demo",
                .size = {.width = 800, .height = 600}},
            logo,
            kUntilWindowClosed);
    }
    catch (const std::exception &error)
    {
        std::cerr << "antwika_gfx_demo: " << error.what() << '\n';
        exitCode = EXIT_FAILURE;
    }

    return exitCode;
}
