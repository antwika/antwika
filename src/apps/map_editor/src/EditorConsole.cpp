#include "antwika/map_editor/EditorConsole.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string_view>
#include <variant>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/Key.hpp>

#include "antwika/map_editor/Commands.hpp"
#include "antwika/map_editor/Generate.hpp"
#include "antwika/map_editor/PaletteMath.hpp"

namespace antwika::map_editor
{

    namespace
    {
        constexpr std::uint32_t kCanvasHeight = 270;

        struct Split final
        {
            std::string word;
            std::string rest;
        };

        [[nodiscard]] Split splitFirst(const std::string &line)
        {
            const auto space = line.find(' ');

            Split split;

            split.word = line.substr(0, space);

            if (space != std::string::npos)
            {
                const auto rest = line.substr(space + 1);
                const auto first = rest.find_first_not_of(' ');

                if (first != std::string::npos)
                {
                    const auto last = rest.find_last_not_of(' ');

                    split.rest =
                        rest.substr(first, last - first + 1);
                }
            }

            return split;
        }

        [[nodiscard]] std::optional<std::uint32_t> parseNumber(
            const std::string &text)
        {
            std::uint32_t value = 0;
            const auto *end = text.data() + text.size();
            const auto result =
                std::from_chars(text.data(), end, value);

            if (result.ec != std::errc{} || result.ptr != end)
            {
                return std::nullopt;
            }

            return value;
        }

        constexpr std::array<std::string_view, 7> kHelpLines{
            "help - list the commands",
            "open <path> - load a map file",
            "save [<path>] - save the map",
            "generate [<seed>] - run generation",
            "validate - run the validator now",
            "scale <2|3|4> - set the ui scale",
            "palette <ink|paper> <#rrggbb> - recolor the map"};
    }

    EditorCommands::EditorCommands(
        EditorStore &store, log::ILogger &logger)
        : store(store), logger(logger)
    {
    }

    void EditorCommands::execute(
        const std::string &command,
        console::ConsoleState &console)
    {
        const auto asked = splitFirst(command);

        if (asked.word == "help")
        {
            for (const auto &line : kHelpLines)
            {
                console.pushHistory(std::string(line));
            }

            console.pushHistory("quit - close the editor");
            return;
        }

        if (asked.word == "open")
        {
            open(asked.rest, console);
            return;
        }

        if (asked.word == "save")
        {
            save(asked.rest, console);
            return;
        }

        if (asked.word == "generate")
        {
            runGenerate(asked.rest, console);
            return;
        }

        if (asked.word == "validate")
        {
            validate(console);
            return;
        }

        if (asked.word == "scale")
        {
            setScale(asked.rest, console);
            return;
        }

        if (asked.word == "palette")
        {
            setPaletteColor(asked.rest, console);
            return;
        }

        console.pushHistory(
            "unknown command: " + asked.word + " - try help");
    }

    std::vector<std::string> EditorCommands::names() const
    {
        return {
            "help",
            "open",
            "save",
            "generate",
            "validate",
            "scale",
            "palette"};
    }

    void EditorCommands::open(
        const std::string &arguments,
        console::ConsoleState &console)
    {
        if (arguments.empty())
        {
            console.pushHistory("open: name a file to load");
            return;
        }

        const auto error =
            openMapAt(store.state, arguments, logger);

        if (error.has_value())
        {
            console.pushHistory("open: " + *error);
            return;
        }

        store.ui.selected.reset();
        loadEntityBuffers(store);
        console.pushHistory("opened " + arguments);
    }

    void EditorCommands::save(
        const std::string &arguments,
        console::ConsoleState &console)
    {
        const auto path = arguments.empty()
                              ? store.state.path
                              : std::filesystem::path(arguments);
        const auto error = saveMapAt(store.state, path, logger);

        if (error.has_value())
        {
            console.pushHistory("save: " + *error);
            return;
        }

        console.pushHistory("saved " + path.string());
    }

    void EditorCommands::runGenerate(
        const std::string &arguments,
        console::ConsoleState &console)
    {
        if (!arguments.empty())
        {
            const auto seed = parseNumber(arguments);

            if (!seed.has_value())
            {
                console.pushHistory(
                    "generate: the seed is a number");
                return;
            }

            store.state.generateSeed = *seed;
        }

        const auto seedUsed = store.state.generateSeed;

        store.state.generateFailedTicks = 0;
        generate(store.state, logger);

        if (store.state.generateFailedTicks > 0)
        {
            console.pushHistory(
                "generate failed (seed "
                + std::to_string(seedUsed) + ")");
            return;
        }

        console.pushHistory(
            "generated (seed " + std::to_string(seedUsed) + ")");
    }

    void EditorCommands::validate(console::ConsoleState &console)
    {
        validateNow(store.state);

        const auto findings =
            store.state.report.has_value()
                ? store.state.report->findings.size()
                : 0;

        console.pushHistory(
            "validated: " + std::to_string(findings)
            + " finding(s)");
    }

    void EditorCommands::setScale(
        const std::string &arguments,
        console::ConsoleState &console)
    {
        const auto scale = parseNumber(arguments);

        if (!scale.has_value() || *scale < 2 || *scale > 4)
        {
            console.pushHistory("scale: say scale <2|3|4>");
            return;
        }

        store.pendingUiScale = *scale;
        console.pushHistory(
            "scale set to " + std::to_string(*scale) + "x");
    }

    void EditorCommands::setPaletteColor(
        const std::string &arguments,
        console::ConsoleState &console)
    {
        const auto split = splitFirst(arguments);
        const auto color = rgbOfHex(split.rest);

        const bool ink = split.word == "ink";
        const bool paper = split.word == "paper";

        if ((!ink && !paper) || !color.has_value())
        {
            console.pushHistory(
                "palette: say palette <ink|paper> <#rrggbb>");
            return;
        }

        const auto &header = store.state.map.header();

        setPalette(
            store.state,
            ink ? *color : header.ink,
            paper ? *color : header.paper);
        console.pushHistory(
            "palette " + split.word + " set to " + split.rest);
    }

    EditorConsoleSystem::EditorConsoleSystem(
        EditorStore &store, log::ILogger &logger)
        : store(store),
          fold(codec),
          overlay(store.windowSize),
          commands(store, logger),
          quit(store),
          sink(console::ConsoleSinkSetup{
              .console = state,
              .input = fold,
              .picture = overlay,
              .scene = scene,
              .controls = controls,
              .commands = commands,
              .stop = quit})
    {
    }

    void EditorConsoleSystem::update(
        ecs::World &, const antwika::time::Tick tick)
    {
        if (overlay.canvas() != store.windowSize)
        {
            overlay = console::ConsolePicture(store.windowSize);
        }

        const event::TickEvent frameTick{
            .tick = tick,
            .event =
                event::Event{.name = antwika::engine::events::kTick}};

        fold.handle(frameTick);
        sink.handle(frameTick);

        for (const auto &event : store.input.events)
        {
            const event::TickEvent ticked{
                .tick = tick, .event = codec.encode(event)};

            fold.handle(ticked);
            sink.handle(ticked);

            const auto *pressed =
                std::get_if<input::KeyPressed>(&event);

            if (pressed != nullptr && state.visible()
                && pressed->key == input::Key::Escape
                && !pressed->repeat)
            {
                state.toggle();
            }
        }

        store.input.consoleVisible = state.visible();
        store.input.consoleHeightCanvas = static_cast<std::int32_t>(
            state.height() * kCanvasHeight
            / std::max(1U, store.windowSize.height));
    }

    const console::ConsolePicture &
    EditorConsoleSystem::picture() const noexcept
    {
        return overlay;
    }

}
