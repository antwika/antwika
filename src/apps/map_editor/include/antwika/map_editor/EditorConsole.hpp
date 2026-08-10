#pragma once

#include <string>
#include <vector>

#include <antwika/console/ConsolePicture.hpp>
#include <antwika/console/ConsoleScene.hpp>
#include <antwika/console/ConsoleSink.hpp>
#include <antwika/console/ConsoleState.hpp>
#include <antwika/console/IConsoleCommands.hpp>
#include <antwika/console/IConsoleControls.hpp>
#include <antwika/console/InputFold.hpp>
#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/map_editor/EditorStore.hpp"

namespace antwika::map_editor
{

    class EditorCommands final : public console::IConsoleCommands
    {
    public:
        EditorCommands(EditorStore &store, log::ILogger &logger);

        EditorCommands(const EditorCommands &) = delete;
        EditorCommands(EditorCommands &&) = delete;

        EditorCommands &operator=(const EditorCommands &) = delete;
        EditorCommands &operator=(EditorCommands &&) = delete;

        void execute(
            const std::string &command,
            console::ConsoleState &console) override;

        [[nodiscard]] std::vector<std::string> names() const override;

    private:
        void open(
            const std::string &arguments,
            console::ConsoleState &console);

        void save(
            const std::string &arguments,
            console::ConsoleState &console);

        void runGenerate(
            const std::string &arguments,
            console::ConsoleState &console);

        void validate(console::ConsoleState &console);

        void setScale(
            const std::string &arguments,
            console::ConsoleState &console);

        void setPaletteColor(
            const std::string &arguments,
            console::ConsoleState &console);

        EditorStore &store;
        log::ILogger &logger;
    };

    class EditorConsoleSystem final : public ecs::ISystem
    {
    public:
        EditorConsoleSystem(EditorStore &store, log::ILogger &logger);

        EditorConsoleSystem(const EditorConsoleSystem &) = delete;
        EditorConsoleSystem(EditorConsoleSystem &&) = delete;

        EditorConsoleSystem &operator=(const EditorConsoleSystem &) =
            delete;
        EditorConsoleSystem &operator=(EditorConsoleSystem &&) =
            delete;

        void update(
            ecs::World &world, antwika::time::Tick tick) override;

        [[nodiscard]] const console::ConsolePicture &
        picture() const noexcept;

    private:
        class QuitSink final : public event::ITickEventSink
        {
        public:
            explicit QuitSink(EditorStore &store) noexcept
                : store(store)
            {
            }

            void handle(const event::TickEvent &) override
            {
                store.input.quit = true;
            }

        private:
            EditorStore &store;
        };

        EditorStore &store;
        input::InputEventCodec codec{};
        console::InputFold fold;
        console::ConsolePicture overlay;
        console::ConsoleState state{};
        const console::ConsoleScene scene{};
        const console::FixedConsoleControls controls{};
        EditorCommands commands;
        QuitSink quit;
        console::ConsoleSink sink;
    };

}
