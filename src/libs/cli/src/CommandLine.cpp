#include "antwika/cli/CommandLine.hpp"

#include <algorithm>
#include <utility>
#include <vector>

#include "antwika/cli/CommandLineError.hpp"

namespace antwika::cli
{

    namespace
    {
        const FlagSpec &helpSpec()
        {
            static const FlagSpec spec{
                .name = kHelpFlag,
                .valueName = {},
                .help = "Show this message and exit.",
            };
            return spec;
        }

        const FlagSpec *findFlag(
            std::span<const FlagSpec> flags, std::string_view name)
        {
            if (name == kHelpFlag)
            {
                return &helpSpec();
            }

            const auto foundFlag = std::ranges::find(
                flags, name, &FlagSpec::name);
            return foundFlag == flags.end() ? nullptr : &*foundFlag;
        }

        std::size_t widestFlag(std::span<const FlagSpec> flags)
        {
            std::size_t widest = kHelpFlag.size();
            for (const auto &flag : flags)
            {
                std::size_t width = flag.name.size();
                if (!flag.valueName.empty())
                {
                    width += 1 + flag.valueName.size();
                }
                widest = std::max(widest, width);
            }
            return widest;
        }

        void appendFlagLine(
            std::string &text, const FlagSpec &flag, std::size_t width)
        {
            std::string left(flag.name);
            if (!flag.valueName.empty())
            {
                left += ' ';
                left += flag.valueName;
            }

            text += "  ";
            text += left;
            text.append(width - left.size(), ' ');
            text += "  ";
            text += flag.help;
            text += '\n';
        }
    }

    CommandLine::CommandLine(CommandLine::Values values) noexcept
        : values(std::move(values))
    {
    }

    bool CommandLine::has(std::string_view flag) const
    {
        return values.find(flag) != values.end();
    }

    std::optional<std::string> CommandLine::value(
        std::string_view flag) const
    {
        const auto foundFlag = values.find(flag);
        if (foundFlag == values.end())
        {
            return std::nullopt;
        }
        return foundFlag->second;
    }

    CommandLine parseCommandLine(
        int argc, char **argv, std::span<const FlagSpec> flags)
    {
        CommandLine::Values values;
        for (int i = 1; i < argc; ++i)
        {
            const std::string_view argument = argv[i];

            const FlagSpec *flag = findFlag(flags, argument);
            if (flag == nullptr)
            {
                throw CommandLineError(
                    "antwika::cli: unrecognised argument: "
                    + std::string(argument));
            }

            if (flag->valueName.empty())
            {
                values[std::string(argument)] = std::string();
                continue;
            }

            if (i + 1 >= argc)
            {
                throw CommandLineError(
                    "antwika::cli: " + std::string(argument)
                    + " needs a value");
            }

            values[std::string(argument)] = argv[++i];
        }
        return CommandLine(std::move(values));
    }

    std::string helpText(
        std::string_view program, std::span<const FlagSpec> flags)
    {
        const std::size_t width = widestFlag(flags);

        std::string text = "Usage: ";
        text += program;
        text += " [options]\n\n";

        for (const auto &flag : flags)
        {
            appendFlagLine(text, flag, width);
        }
        appendFlagLine(text, helpSpec(), width);

        return text;
    } // GCOVR_EXCL_LINE

}
