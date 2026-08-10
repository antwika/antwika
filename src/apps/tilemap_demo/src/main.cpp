#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <variant>

#include <antwika/app/ConsoleLogging.hpp>
#include <antwika/app/RunGuarded.hpp>
#include <antwika/app/WindowEvents.hpp>
#include <antwika/console/ConsolePicture.hpp>
#include <antwika/console/ConsoleScene.hpp>
#include <antwika/console/ConsoleSink.hpp>
#include <antwika/console/ConsoleState.hpp>
#include <antwika/console/IConsoleControls.hpp>
#include <antwika/console/InputFold.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/autotile/DrawPlan.hpp>
#include <antwika/autotile/SheetLayout.hpp>
#include <antwika/autotile/TilePiece.hpp>
#include <antwika/cli/CommandLine.hpp>
#include <antwika/enums/Enumeration.hpp>
#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/GfxError.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/PngReader.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/SelectedBackend.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/SelectedInputBackend.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/tilemap/MapFile.hpp>
#include <antwika/tilemap/Rgb.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/tilemap/TileMap.hpp>
#include <antwika/time/SystemSleeper.hpp>
#include <antwika/ui/Painter.hpp>

#include "antwika/tilemap_demo/DemoConsole.hpp"
#include "antwika/tilemap_demo/DemoMap.hpp"
#include "antwika/tilemap_demo/PlaceholderSheets.hpp"

using antwika::app::ConsoleLogging;
using antwika::app::runGuarded;
using antwika::geometry::GridCell;
using antwika::gfx::Color;
using antwika::gfx::RectF;
using antwika::gfx::ViewportRenderer;
using antwika::gfx::WindowDesc;
using antwika::input::Key;
using antwika::input::KeyPressed;
using antwika::log::Level;
using antwika::tilemap::TerrainClass;
using antwika::tilemap::TileMap;
using antwika::tilemap_demo::demoMap;
using antwika::tilemap_demo::placeholderSheet;
using antwika::tilemap_demo::Player;
using antwika::tilemap_demo::walkable;

namespace
{
    constexpr std::chrono::milliseconds kFramePeriod{16};

    constexpr std::string_view kName = "antwika_tilemap_demo";

    constexpr std::array kFlags = {
        antwika::cli::FlagSpec{
            .name = "--map",
            .valueName = "path",
            .help = "Load a map JSON file instead of the built-in map."},
    };

    constexpr antwika::gfx::Size kCanvas{.width = 320, .height = 180};

    constexpr antwika::gfx::Size kWindow{.width = 1280, .height = 720};

    constexpr Color kMarker{.red = 255, .green = 176, .blue = 64};

    constexpr Color kWhite{.red = 255, .green = 255, .blue = 255};

    [[nodiscard]] Color colorOf(const antwika::tilemap::Rgb rgb)
    {
        return Color{
            .red = rgb.red, .green = rgb.green, .blue = rgb.blue};
    }

    constexpr std::uint32_t kWalkTicks = 12;

    class CloseSink final : public antwika::event::ITickEventSink
    {
    public:
        explicit CloseSink(antwika::gfx::IWindow &window) noexcept
            : window(window)
        {
        }

        void handle(const antwika::event::TickEvent &) override
        {
            window.close();
        }

    private:
        antwika::gfx::IWindow &window;
    };

    void move(
        const TileMap &map,
        Player &player,
        const std::int64_t byColumn,
        const std::int64_t byRow)
    {
        if (byRow > 0)
        {
            player.direction = 0;
        }
        else if (byRow < 0)
        {
            player.direction = 1;
        }
        else if (byColumn < 0)
        {
            player.direction = 2;
        }
        else if (byColumn > 0)
        {
            player.direction = 3;
        }

        const auto column =
            static_cast<std::int64_t>(player.cell.column) + byColumn;
        const auto row =
            static_cast<std::int64_t>(player.cell.row) + byRow;

        if (column < 0 || row < 0
            || column >= map.columns() || row >= map.rows())
        {
            return;
        }

        const auto target = GridCell{
            .column = static_cast<std::uint32_t>(column),
            .row = static_cast<std::uint32_t>(row)};

        if (!walkable(map, target))
        {
            return;
        }

        player.cell = target;
        player.height = map.at(target).height;
        player.moveTicks = kWalkTicks;
    }

    [[nodiscard]] std::unique_ptr<antwika::gfx::ITexture>
    loadPlayerSheet(
        ViewportRenderer &view, antwika::log::ILogger &logger)
    {
        const std::filesystem::path path =
            "assets/characters/player.png";

        if (!std::filesystem::is_regular_file(path))
        {
            return nullptr;
        }

        try
        {
            std::ifstream in(path, std::ios::binary);
            const auto bitmap = antwika::gfx::PngReader{}.read(in);

            if (bitmap.size.width != 64 || bitmap.size.height != 64)
            {
                logger.log(
                    Level::Warning,
                    "tilemap_demo: player.png is not 64x64");
                return nullptr;
            }

            logger.log(Level::Info, "Loaded " + path.string());

            return view.createTexture(bitmap);
        }
        catch (const antwika::gfx::GfxError &error)
        {
            logger.log(Level::Warning, error.what());
            return nullptr;
        }
    }

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
                antwika::cli::parseCommandLine(argc, argv, kFlags);

            if (command.has(antwika::cli::kHelpFlag))
            {
                std::cout << antwika::cli::helpText(kName, kFlags);
                return;
            }

            const auto backend = antwika::gfx::makeSelectedBackend(logger);
            const auto input =
                antwika::input::makeSelectedInputBackend(logger);

            const auto window = backend->createWindow(WindowDesc{
                .title = "Wakewater tilemap demo",
                .size = kWindow});

            ViewportRenderer view(
                window->renderer(), window->size(), kCanvas);

            auto map = demoMap();

            if (const auto path = command.value("--map"))
            {
                map = antwika::tilemap::loadMapFile(*path);

                logger.log(Level::Info, "Loaded map: " + *path);
            }

            std::array<
                std::unique_ptr<antwika::gfx::ITexture>,
                antwika::enums::kCount<TerrainClass>>
                sheets;

            for (const auto terrain :
                 antwika::enums::kAll<TerrainClass>)
            {
                sheets[antwika::enums::index(terrain)] =
                    view.createTexture(
                        placeholderSheet(terrain, kWhite));
            }

            Player player;
            antwika::time::SystemSleeper sleeper;
            std::uint32_t clock = 0;

            const auto playerSheet = loadPlayerSheet(view, logger);

            const antwika::input::InputEventCodec codec;
            antwika::console::InputFold fold(codec);
            antwika::console::ConsolePicture overlay(kWindow);
            antwika::console::ConsoleState consoleState;
            const antwika::console::ConsoleScene consoleScene{};
            const antwika::console::FixedConsoleControls controls;
            antwika::tilemap_demo::DemoCommands commands(
                map, player, logger);
            CloseSink closeSink(*window);
            antwika::console::ConsoleSink consoleSink(
                antwika::console::ConsoleSinkSetup{
                    .console = consoleState,
                    .input = fold,
                    .picture = overlay,
                    .scene = consoleScene,
                    .controls = controls,
                    .commands = commands,
                    .stop = closeSink});

            while (window->isOpen())
            {
                if (antwika::app::closeRequestedOn(
                        *backend, window->id()))
                {
                    window->close();
                    break;
                }

                const antwika::event::TickEvent frameTick{
                    .tick = clock,
                    .event = antwika::event::Event{
                        .name = antwika::engine::events::kTick}};

                fold.handle(frameTick);
                consoleSink.handle(frameTick);

                while (const auto event = input->pollEvent())
                {
                    const antwika::event::TickEvent ticked{
                        .tick = clock,
                        .event = codec.encode(event.value())};

                    fold.handle(ticked);
                    consoleSink.handle(ticked);

                    const auto *pressed =
                        std::get_if<KeyPressed>(&event.value());

                    if (pressed == nullptr)
                    {
                        continue;
                    }

                    if (consoleState.visible())
                    {
                        if (pressed->key == Key::Escape
                            && !pressed->repeat)
                        {
                            consoleState.toggle();
                        }

                        continue;
                    }

                    if (pressed->key == Key::Escape)
                    {
                        window->close();
                    }

                    if (pressed->key == Key::ArrowUp)
                    {
                        move(map, player, 0, -1);
                    }

                    if (pressed->key == Key::ArrowDown)
                    {
                        move(map, player, 0, 1);
                    }

                    if (pressed->key == Key::ArrowLeft)
                    {
                        move(map, player, -1, 0);
                    }

                    if (pressed->key == Key::ArrowRight)
                    {
                        move(map, player, 1, 0);
                    }
                }

                if (!window->isOpen())
                {
                    break;
                }

                const auto ink = colorOf(map.header().ink);
                const auto paper = colorOf(map.header().paper);

                view.clear(paper);
                view.fillSurround(Color{});

                const auto plan = antwika::autotile::buildDrawPlan(
                    map, player.cell, player.height, clock);

                for (const auto &draw : plan)
                {
                    const auto source = antwika::autotile::sheetSource(
                        draw.piece, draw.mask, draw.variant);

                    const auto shade =
                        draw.piece == antwika::autotile::TilePiece::Shade;

                    view.drawTexture(
                        *sheets[antwika::enums::index(draw.terrain)],
                        source,
                        RectF(
                            {static_cast<float>(draw.screen.x),
                             static_cast<float>(draw.screen.y)},
                            {8.0F, 8.0F}),
                        shade ? Color{.red = 0, .green = 0, .blue = 0}
                              : ink);
                }

                if (playerSheet != nullptr)
                {
                    const auto frame =
                        player.moveTicks > 0
                            ? static_cast<float>((clock / 8) % 4)
                            : 0.0F;

                    view.drawTexture(
                        *playerSheet,
                        RectF(
                            {frame * 16.0F,
                             static_cast<float>(player.direction)
                                 * 16.0F},
                            {16.0F, 16.0F}),
                        RectF(
                            {static_cast<float>(player.cell.column)
                                 * 16.0F,
                             static_cast<float>(player.cell.row)
                                     * 16.0F
                                 - static_cast<float>(player.height)
                                       * 8.0F},
                            {16.0F, 16.0F}),
                        kWhite);
                }
                else
                {
                    const auto markerX =
                        static_cast<float>(player.cell.column)
                            * 16.0F
                        + 4.0F;
                    const auto markerY =
                        static_cast<float>(player.cell.row) * 16.0F
                        + 4.0F
                        - static_cast<float>(player.height) * 8.0F;

                    view.drawRect(
                        RectF({markerX, markerY}, {8.0F, 8.0F}),
                        kMarker);
                }

                if (player.moveTicks > 0)
                {
                    --player.moveTicks;
                }

                view.drawText(
                    {4.0F, 4.0F},
                    "arrows move - esc quits",
                    1,
                    ink);

                antwika::ui::paint(
                    window->renderer(), overlay.commands());

                view.present();
                sleeper.sleep(kFramePeriod);
                ++clock;
            }

            window->close();

            logger.log(Level::Info, "Wakewater tilemap demo closed");
        });
}
