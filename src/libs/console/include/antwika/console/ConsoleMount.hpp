#pragma once

#include <functional>
#include <optional>
#include <string>

#include <antwika/event/ITickEventSink.hpp>

#include "antwika/console/ConsoleEvents.hpp"
#include "antwika/console/ConsoleGatedSink.hpp"
#include "antwika/console/ConsolePicture.hpp"
#include "antwika/console/ConsoleScene.hpp"
#include "antwika/console/ConsoleSink.hpp"
#include "antwika/console/ConsoleState.hpp"
#include "antwika/console/IConsoleControls.hpp"
#include "antwika/console/ISnapshotStore.hpp"
#include "antwika/console/InputFold.hpp"
#include "antwika/console/SnapshotCommands.hpp"

namespace antwika::console
{

    using antwika::event::ITickEventSink;

    struct ConsoleMountSetup final
    {
        std::optional<std::reference_wrapper<ConsolePicture>> overlay;

        InputFold &input;

        ISnapshotStore &store;

        const std::string &dumpPath;

        bool loadEnabled;

        std::optional<std::reference_wrapper<const IConsoleControls>>
            controls = std::nullopt;

        std::optional<std::reference_wrapper<ITickEventSink>> stop =
            std::nullopt;
    };

    class ConsoleMount final
    {
    public:
        explicit ConsoleMount(const ConsoleMountSetup &setup);

        ConsoleMount(const ConsoleMount &) = delete;
        ConsoleMount(ConsoleMount &&) = delete;

        ConsoleMount &operator=(const ConsoleMount &) = delete;
        ConsoleMount &operator=(ConsoleMount &&) = delete;

        [[nodiscard]] bool mounted() const noexcept;

        [[nodiscard]] ConsoleSink &sink() noexcept;

        [[nodiscard]] ConsoleState &state() noexcept;

        [[nodiscard]] ConsoleGatedSink gate(
            ITickEventSink &inner) noexcept;

        [[nodiscard]] ConsoleEvents &events() noexcept;

    private:
        ConsolePicture noConsole;
        bool isMounted;
        ConsolePicture &picture;
        InputFold &input;
        ConsoleState console;
        const ConsoleScene scene{};
        const FixedConsoleControls fixedControls;
        const IConsoleControls &controls;
        SnapshotCommands commands;
        ConsoleEvents sent;
        ConsoleSink consoleSink;
    };

}
