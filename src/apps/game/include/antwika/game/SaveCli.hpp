#pragma once

#include <optional>
#include <span>
#include <string>

#include <antwika/cli/CommandLine.hpp>
#include <antwika/cli/FlagSpec.hpp>

namespace antwika::game
{

    struct SaveCliOptions final
    {
        std::optional<std::string> savePath;

        std::optional<std::string> loadPath;
    };

    [[nodiscard]] std::span<const antwika::cli::FlagSpec> saveCliFlags();

    [[nodiscard]] SaveCliOptions saveCliOptionsFrom(
        const antwika::cli::CommandLine &parsed);

    void requireRecordableStart(
        const SaveCliOptions &options, bool recording);

}
