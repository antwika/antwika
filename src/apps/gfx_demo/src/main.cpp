#include <cstdint>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/PngReader.hpp>
#include <antwika/gfx/SelectedBackend.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/log/Logger.hpp>
#include <antwika/log/MinimumLevelLogPolicy.hpp>
#include <antwika/log/PlainFormatter.hpp>
#include <antwika/log/StreamAppender.hpp>
#include <antwika/time/SystemClock.hpp>

#include "antwika/gfx_demo/DemoLoop.hpp"
#include "antwika/gfx_demo/DemoScene.hpp"

using antwika::gfx::PngReader;
using antwika::gfx::WindowDesc;
using antwika::gfx_demo::DemoLoop;
using antwika::gfx_demo::DemoScene;
using antwika::log::Level;
using antwika::log::Logger;
using antwika::log::MinimumLevelLogPolicy;
using antwika::log::PlainFormatter;
using antwika::log::StreamAppender;
using antwika::time::SystemClock;

namespace
{
    // The demo is something to look at, so it draws until it is closed.
    // The null backend reports no close, so that build never finishes.
    constexpr std::optional<std::uint32_t> kUntilWindowClosed = std::nullopt;
} // namespace

int main()
{
    SystemClock clock;
    StreamAppender appender(std::cout);
    PlainFormatter formatter;
    MinimumLevelLogPolicy logPolicy(Level::Info);
    Logger logger(formatter, logPolicy, clock, appender);

    const auto backend = antwika::gfx::makeSelectedBackend(logger);

    logger.log(
        Level::Info,
        "Antwika gfx demo on backend: " + std::string(backend->name()));

    // Opening the file is the application's job, not the library's.
    // antwika::gfx decodes bytes and never goes looking for them.
    std::ifstream file(ANTWIKA_GFX_DEMO_TEXTURE_PATH, std::ios::binary);
    const auto logo = PngReader{}.read(file);

    const DemoScene scene;
    DemoLoop loop(*backend, scene);

    loop.run(
        WindowDesc{
            .title = "Antwika gfx demo",
            .size = {.width = 800, .height = 600}},
        logo,
        kUntilWindowClosed);

    return 0;
}
