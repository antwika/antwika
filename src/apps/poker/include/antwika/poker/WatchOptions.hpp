#pragma once

#include <chrono>
#include <span>

#include <antwika/cli/CommandLine.hpp>
#include <antwika/cli/FlagSpec.hpp>

namespace antwika::poker
{

    inline constexpr std::chrono::milliseconds kDefaultTickDelay{1000};

    struct WatchOptions final
    {
        std::chrono::milliseconds tickDelay{kDefaultTickDelay};

        bool holdFinalFrame{false};

        bool operator==(const WatchOptions &other) const = default;
    };

    [[nodiscard]] std::span<const antwika::cli::FlagSpec> watchFlags();

    [[nodiscard]] WatchOptions watchOptionsFrom(
        const antwika::cli::CommandLine &parsed);

}
