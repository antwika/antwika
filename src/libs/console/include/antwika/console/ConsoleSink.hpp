#pragma once

#include <functional>
#include <optional>
#include <string>

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/ui/Interactions.hpp>
#include <antwika/ui/Keyboard.hpp>
#include <antwika/ui/Pointer.hpp>

#include "antwika/console/ConsolePicture.hpp"
#include "antwika/console/ConsoleEvents.hpp"
#include "antwika/console/ConsoleScene.hpp"
#include "antwika/console/ConsoleState.hpp"
#include "antwika/console/IConsoleCommands.hpp"
#include "antwika/console/IConsoleControls.hpp"
#include "antwika/console/InputFold.hpp"

namespace antwika::console
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::ui::Interactions;
    using antwika::ui::Keyboard;
    using antwika::ui::Pointer;

    struct ConsoleSinkSetup final
    {
        ConsoleState &console;

        const InputFold &input;

        ConsolePicture &picture;

        const ConsoleScene &scene;

        const IConsoleControls &controls;

        IConsoleCommands &commands;

        std::optional<std::reference_wrapper<ITickEventSink>> stop =
            std::nullopt;

        std::optional<std::reference_wrapper<ConsoleEvents>> events =
            std::nullopt;
    };

    class ConsoleSink final : public ITickEventSink
    {
    public:
        explicit ConsoleSink(const ConsoleSinkSetup &setup);

        ConsoleSink(const ConsoleSink &) = delete;
        ConsoleSink(ConsoleSink &&) = delete;

        ConsoleSink &operator=(const ConsoleSink &) = delete;
        ConsoleSink &operator=(ConsoleSink &&) = delete;

        void handle(const TickEvent &event) override;

    private:
        [[nodiscard]] Pointer pointerNow(bool pressed) const;

        void refreshAndAct(
            antwika::time::Tick tick,
            bool pressed,
            const Keyboard &keyboard);

        void act(
            antwika::time::Tick tick, const Interactions &interactions);

        void execute(
            antwika::time::Tick tick, const std::string &command);

        void send(const std::string &arguments);

        void listCommands(const std::string &arguments);

        void reportRefusals();

        ConsoleSinkSetup setup;
    };

}
