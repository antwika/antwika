#pragma once

#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include <antwika/cli/FlagSpec.hpp>

namespace antwika::cli
{

    class CommandLine final
    {
    public:
        using Values = std::map<std::string, std::string, std::less<>>;

        explicit CommandLine(Values givenValues) noexcept;

        [[nodiscard]] bool has(std::string_view flag) const;

        [[nodiscard]] std::optional<std::string> value(
            std::string_view flag) const;

    private:
        Values values;
    };

    inline constexpr std::string_view kHelpFlag = "--help";

    [[nodiscard]] CommandLine parseCommandLine(
        int argc, char **argv, std::span<const FlagSpec> flags);

    [[nodiscard]] std::string helpText(
        std::string_view program, std::span<const FlagSpec> flags);

}
