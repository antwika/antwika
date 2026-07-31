#pragma once

#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include <antwika/cli/FlagSpec.hpp>

namespace antwika::cli
{

    /**
     * @brief What a program was invoked with, once every flag has been
     * matched against the table of flags that program accepts.
     *
     * A flag given more than once keeps the last value, which is what a
     * shell history full of edited commands makes the useful answer.
     */
    class CommandLine final
    {
    public:
        /**
         * @brief The name a value-less flag maps to when it was given.
         *
         * Its own type so that has() and value() cannot be confused: a
         * flag that takes no value is present or absent, never empty.
         */
        using Values = std::map<std::string, std::string, std::less<>>;

        /**
         * @brief Construct from what a parse worked out.
         * @param given Every flag that was given, mapped to its value;
         * a flag taking no value maps to an empty string.
         */
        explicit CommandLine(Values given) noexcept;

        /**
         * @brief Ask whether a flag was given at all.
         * @param flag The flag's name, leading dashes and all.
         * @return True when it appeared on the command line.
         */
        [[nodiscard]] bool has(std::string_view flag) const;

        /**
         * @brief Get the value a flag was given.
         * @param flag The flag's name, leading dashes and all.
         * @return The value, or nothing if the flag was not given.
         * A flag that takes no value yields an empty string when it was
         * given, so reach for has() rather than this one there.
         */
        [[nodiscard]] std::optional<std::string> value(
            std::string_view flag) const;

    private:
        Values given;
    };

    /**
     * @brief The flag every program accepts without having to say so.
     *
     * Declared here so that no table can forget it, and so that a
     * program cannot document a --help it does not accept.
     */
    inline constexpr std::string_view kHelpFlag = "--help";

    /**
     * @brief Parse a command line against the flags a program accepts.
     * @param argc Argument count, as passed to `main()`.
     * @param argv Argument vector, as passed to `main()`; argv[0] is the
     * program name and is not parsed.
     * @param flags The flags this program accepts, besides `--help`.
     * @return What was given.
     * @throws CommandLineError If an argument is not a flag in the
     * table, or if a flag that takes a value is the last argument.
     * Both used to be ignored, which is how `--replya demo.json` started
     * an empty session instead of saying it was a typo.
     */
    [[nodiscard]] CommandLine parseCommandLine(
        int argc, char **argv, std::span<const FlagSpec> flags);

    /**
     * @brief Render the `--help` text for a program from the same table
     * its command line is parsed against.
     * @param program The program's name, as it is run.
     * @param flags The flags this program accepts, besides `--help`,
     * which is appended for you.
     * @return The help text, ending in a newline.
     */
    [[nodiscard]] std::string helpText(
        std::string_view program, std::span<const FlagSpec> flags);

} // namespace antwika::cli
