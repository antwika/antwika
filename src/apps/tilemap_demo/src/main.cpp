#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <antwika/app/ConsoleLogging.hpp>
#include <antwika/app/RunGuarded.hpp>
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
#include <antwika/autotile/SystemSheet.hpp>
#include <antwika/autotile/TileDraw.hpp>
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
#include <antwika/gfx/WindowEvent.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/SelectedInputBackend.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/tilemap/MapFile.hpp>
#include <antwika/tilemap/Rgb.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/tilemap/TileMap.hpp>
#include <antwika/tileset/Atlas.hpp>
#include <antwika/tileset/Tileset.hpp>
#include <antwika/tileset/TilesetFile.hpp>
#include <antwika/time/SystemSleeper.hpp>
#include <antwika/ui/Painter.hpp>

#include "antwika/tilemap_demo/DemoConsole.hpp"
#include "antwika/tilemap_demo/DemoMap.hpp"
#include "antwika/tilemap_demo/PlaceholderTilesets.hpp"

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
using antwika::tilemap_demo::landingLevel;
using antwika::tilemap_demo::placeholderSystemSheet;
using antwika::tilemap_demo::placeholderTileset;
using antwika::tilemap_demo::Player;
using antwika::tilemap_demo::restingLevel;

namespace
{
    constexpr std::chrono::milliseconds kFramePeriod{16};

    constexpr std::string_view kName = "antwika_tilemap_demo";

    constexpr std::array kFlags = {
        antwika::cli::FlagSpec{
            .name = "--map",
            .valueName = "path",
            .help = "Load a map JSON file instead of the built-in map."},
        antwika::cli::FlagSpec{
            .name = "--tilesets",
            .valueName = "dir",
            .help = "Load tilesets from this directory instead of "
                    "assets/tilesets."},
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

    [[nodiscard]] antwika::gfx::Bitmap bakedSheet(
        const antwika::gfx::Bitmap &sheet,
        const Color ink,
        const Color paper)
    {
        auto baked = sheet;

        for (std::size_t at = 0; at + 3 < baked.pixels.size();
             at += 4)
        {
            if (baked.pixels[at + 3] == 0)
            {
                continue;
            }

            const auto luminance = (54U * baked.pixels[at]
                                    + 183U * baked.pixels[at + 1]
                                    + 19U * baked.pixels[at + 2])
                                   / 256U;
            const auto &color = luminance >= 192 ? ink : paper;

            baked.pixels[at] = color.red;
            baked.pixels[at + 1] = color.green;
            baked.pixels[at + 2] = color.blue;
        }

        return baked;
    }

    [[nodiscard]] antwika::gfx::Bitmap loadSystemArt(
        const std::filesystem::path &directory,
        antwika::log::ILogger &logger)
    {
        const auto path = directory / "system.png";

        if (!std::filesystem::is_regular_file(path))
        {
            return placeholderSystemSheet();
        }

        try
        {
            std::ifstream in(path, std::ios::binary);
            const auto bitmap = antwika::gfx::PngReader{}.read(in);

            if (bitmap.size.width != 32 || bitmap.size.height != 8)
            {
                logger.log(
                    Level::Warning,
                    "tilemap_demo: system.png is not 32x8");
                return placeholderSystemSheet();
            }

            logger.log(Level::Info, "Loaded " + path.string());

            return bakedSheet(
                bitmap,
                Color{.red = 255, .green = 255, .blue = 255},
                Color{.red = 128, .green = 128, .blue = 128});
        }
        catch (const antwika::gfx::GfxError &error)
        {
            logger.log(Level::Warning, error.what());
            return placeholderSystemSheet();
        }
    }

    [[nodiscard]] const antwika::tileset::Tileset *findTileset(
        const std::vector<antwika::tileset::Tileset> &library,
        const std::string &name)
    {
        for (const auto &set : library)
        {
            if (set.name == name)
            {
                return &set;
            }
        }

        return nullptr;
    }

    [[nodiscard]] antwika::tileset::Tileset resolveTileset(
        const std::vector<antwika::tileset::Tileset> &library,
        const std::string &bound,
        const TerrainClass terrain)
    {
        const auto *found =
            bound.empty() ? nullptr : findTileset(library, bound);

        if (found == nullptr)
        {
            found = findTileset(
                library,
                "default-"
                    + std::string(
                        antwika::tilemap::toString(terrain)));
        }

        return found != nullptr ? *found
                                : placeholderTileset(terrain);
    }

    [[nodiscard]] antwika::gfx::Bitmap atlasArtOf(
        const antwika::tileset::Tileset &set,
        const Color ink,
        const Color paper)
    {
        if (antwika::tileset::atlasIndexOf(set).rows > 0)
        {
            return antwika::tileset::bakeAtlas(set, ink, paper);
        }

        antwika::gfx::Bitmap blank{
            .size =
                {.width = static_cast<std::uint32_t>(
                     antwika::tileset::kAtlasWidth),
                 .height = static_cast<std::uint32_t>(
                     antwika::tileset::kSpriteSide)},
            .pixels = {}};

        blank.pixels.assign(
            static_cast<std::size_t>(antwika::tileset::kAtlasWidth)
                * antwika::tileset::kSpriteSide
                * antwika::gfx::kBytesPerPixel,
            0);

        return blank;
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
        const auto landed =
            landingLevel(map, player.cell, player.level, target);

        if (!landed.has_value())
        {
            return;
        }

        player.cell = target;
        player.level = *landed;
        player.moveTicks = kWalkTicks;
    }

    [[nodiscard]] std::optional<antwika::gfx::Bitmap>
    loadPlayerSheet(antwika::log::ILogger &logger)
    {
        const std::filesystem::path path =
            "assets/characters/player.png";

        if (!std::filesystem::is_regular_file(path))
        {
            return std::nullopt;
        }

        try
        {
            std::ifstream in(path, std::ios::binary);
            auto bitmap = antwika::gfx::PngReader{}.read(in);

            for (std::size_t at = 0;
                 at + 3 < bitmap.pixels.size();
                 at += 4)
            {
                if (bitmap.pixels[at + 3] == 0)
                {
                    continue;
                }

                const auto luminance =
                    (54U * bitmap.pixels[at]
                     + 183U * bitmap.pixels[at + 1]
                     + 19U * bitmap.pixels[at + 2])
                    / 256U;
                const auto value = luminance >= 192 ? 255 : 128;

                bitmap.pixels[at] =
                    static_cast<std::uint8_t>(value);
                bitmap.pixels[at + 1] =
                    static_cast<std::uint8_t>(value);
                bitmap.pixels[at + 2] =
                    static_cast<std::uint8_t>(value);
            }

            if (bitmap.size.width != 64 || bitmap.size.height != 64)
            {
                logger.log(
                    Level::Warning,
                    "tilemap_demo: player.png is not 64x64");
                return std::nullopt;
            }

            logger.log(Level::Info, "Loaded " + path.string());

            return bitmap;
        }
        catch (const antwika::gfx::GfxError &error)
        {
            logger.log(Level::Warning, error.what());
            return std::nullopt;
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
                .size = kWindow,
                .resizable = true});

            ViewportRenderer view(
                window->renderer(), window->size(), kCanvas);

            auto map = demoMap();

            if (const auto path = command.value("--map"))
            {
                map = antwika::tilemap::loadMapFile(*path);

                logger.log(Level::Info, "Loaded map: " + *path);
            }

            const std::filesystem::path tilesetsDir =
                command.value("--tilesets")
                    .value_or(std::string("assets/tilesets"));
            const auto library =
                antwika::tileset::loadTilesetLibrary(tilesetsDir);

            logger.log(
                Level::Info,
                "Loaded " + std::to_string(library.size())
                    + " tilesets from " + tilesetsDir.string());

            std::array<
                antwika::tileset::Tileset,
                antwika::enums::kCount<TerrainClass>>
                tilesets;
            std::array<
                std::unique_ptr<antwika::gfx::ITexture>,
                antwika::enums::kCount<TerrainClass>>
                atlases;
            antwika::autotile::TilesetBindings bindings{};

            for (const auto terrain :
                 antwika::enums::kAll<TerrainClass>)
            {
                const auto at = antwika::enums::index(terrain);

                bindings.byTerrain[at] = &tilesets[at];
            }

            std::optional<std::array<
                std::string,
                antwika::enums::kCount<TerrainClass>>>
                boundNames;

            const auto systemArt =
                loadSystemArt(tilesetsDir, logger);
            std::unique_ptr<antwika::gfx::ITexture> systemSheet;

            Player player;

            player.level = restingLevel(map.at(player.cell));

            antwika::time::SystemSleeper sleeper;
            std::uint32_t clock = 0;

            const auto playerArt = loadPlayerSheet(logger);
            std::unique_ptr<antwika::gfx::ITexture> playerSheet;
            std::optional<antwika::tilemap::Rgb> bakedInk;
            std::optional<antwika::tilemap::Rgb> bakedPaper;

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
                bool closeRequested = false;

                while (const auto event = backend->pollEvent())
                {
                    if (event->window != window->id())
                    {
                        continue;
                    }

                    if (std::holds_alternative<
                            antwika::gfx::CloseRequested>(
                            event->payload))
                    {
                        closeRequested = true;
                    }

                    if (const auto *resized =
                            std::get_if<antwika::gfx::Resized>(
                                &event->payload))
                    {
                        view.resize(resized->size);
                        overlay = antwika::console::ConsolePicture(
                            resized->size);
                    }
                }

                if (closeRequested)
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

                    if (pressed->key == Key::F10)
                    {
                        window->setFullscreen(
                            !window->isFullscreen());
                        view.resize(window->size());
                        overlay = antwika::console::ConsolePicture(
                            window->size());
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

                if (!boundNames.has_value()
                    || *boundNames != map.header().tilesets)
                {
                    boundNames = map.header().tilesets;

                    for (const auto terrain :
                         antwika::enums::kAll<TerrainClass>)
                    {
                        const auto at =
                            antwika::enums::index(terrain);

                        tilesets[at] = resolveTileset(
                            library,
                            map.header().tilesets[at],
                            terrain);
                    }

                    bakedInk.reset();
                    bakedPaper.reset();
                }

                const auto ink = colorOf(map.header().ink);
                const auto paper = colorOf(map.header().paper);

                if (bakedInk != map.header().ink
                    || bakedPaper != map.header().paper)
                {
                    bakedInk = map.header().ink;
                    bakedPaper = map.header().paper;

                    for (const auto terrain :
                         antwika::enums::kAll<TerrainClass>)
                    {
                        const auto at =
                            antwika::enums::index(terrain);

                        atlases[at] = view.createTexture(
                            atlasArtOf(tilesets[at], ink, paper));
                    }

                    systemSheet = view.createTexture(
                        bakedSheet(systemArt, ink, paper));

                    if (playerArt.has_value())
                    {
                        playerSheet = view.createTexture(bakedSheet(
                            *playerArt, ink, paper));
                    }
                }

                view.clear(paper);
                view.fillSurround(Color{});

                const auto plan = antwika::autotile::buildDrawPlan(
                    map,
                    player.cell,
                    player.level,
                    clock,
                    bindings);

                for (const auto &draw : plan)
                {
                    const bool sprite =
                        draw.kind
                        == antwika::autotile::DrawKind::Sprite;
                    const auto source =
                        sprite ? antwika::tileset::atlasSource(
                            draw.atlasRow, draw.frame)
                               : antwika::autotile::systemSource(
                                   draw.kind);
                    const auto &texture =
                        sprite ? *atlases[antwika::enums::index(
                            draw.terrain)]
                               : *systemSheet;
                    const auto shade =
                        draw.kind
                        == antwika::autotile::DrawKind::Shade;

                    view.drawTexture(
                        texture,
                        source,
                        RectF(
                            {static_cast<float>(draw.screen.x),
                             static_cast<float>(draw.screen.y)},
                            {static_cast<float>(source.size.width),
                             static_cast<float>(
                                 source.size.height)}),
                        shade ? Color{.red = 0, .green = 0, .blue = 0}
                              : kWhite);
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
                                 - static_cast<float>(player.level)
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
                        - static_cast<float>(player.level) * 8.0F;

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
