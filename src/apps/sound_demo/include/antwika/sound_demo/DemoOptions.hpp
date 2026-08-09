#pragma once

#include <optional>
#include <span>
#include <string>

#include <antwika/cli/CommandLine.hpp>
#include <antwika/cli/FlagSpec.hpp>

namespace antwika::sound_demo
{

    struct DemoOptions final
    {
        std::optional<std::string> filePath;

        bool helpRequested = false;
    };

    [[nodiscard]] std::span<const antwika::cli::FlagSpec> demoFlags();

    [[nodiscard]] DemoOptions demoOptionsFrom(
        const antwika::cli::CommandLine &parsed);

}
