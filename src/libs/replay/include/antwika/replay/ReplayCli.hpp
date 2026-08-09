#pragma once

#include <fstream>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/cli/CommandLine.hpp>
#include <antwika/cli/FlagSpec.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/geometry/Size.hpp>
#include <antwika/replay/CanvasCheck.hpp>
#include <antwika/replay/ReplayWriter.hpp>

namespace antwika::replay
{

    using antwika::event::TickEvent;

    struct ReplayCliOptions final
    {
        std::optional<std::string> recordPath;

        std::optional<std::string> replayPath;

        bool helpRequested = false;
    };

    [[nodiscard]] std::span<const cli::FlagSpec> replayCliFlags();

    [[nodiscard]] ReplayCliOptions replayCliOptionsFrom(
        const cli::CommandLine &parsed);

    [[nodiscard]] std::vector<TickEvent> loadReplayFile(
        const std::string &path, CanvasCheck check = {});

    [[nodiscard]] std::ofstream openReplayFile(const std::string &path);

    void saveReplayFile(
        const std::vector<TickEvent> &events,
        const std::string &path,
        std::optional<geometry::Size> canvas = std::nullopt);

}
