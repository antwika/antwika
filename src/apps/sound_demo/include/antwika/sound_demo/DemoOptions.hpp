#pragma once

#include <optional>
#include <span>
#include <string>

#include <antwika/cli/CommandLine.hpp>
#include <antwika/cli/FlagSpec.hpp>

namespace antwika::sound_demo
{

    /**
     * @brief What a demo session was invoked with.
     */
    struct DemoOptions
    {
        /**
         * @brief The WAV file to play, if `--file <path>` was given.
         *
         * Unset means the generated tone, which is what keeps this demo
         * free of an asset checked in beside it.
         */
        std::optional<std::string> filePath;

        /**
         * @brief Whether `--help` was asked for.
         *
         * A program that sees this should print helpText() and stop,
         * rather than run whatever else it was told.
         */
        bool helpRequested = false;
    };

    /**
     * @brief The flags this demo accepts, besides `--help`.
     * @return The table, to parse against and to render help from.
     *
     * The file arrives as a flag rather than as a bare argument
     * because antwika::cli parses flags and nothing else, and this was
     * the one program in the tree reading its own argv instead.
     * What that cost was the two things the library exists to give:
     * `--help` did nothing, and a mistyped flag was taken for a
     * filename and failed much later inside the WAV reader.
     */
    [[nodiscard]] std::span<const antwika::cli::FlagSpec> demoFlags();

    /**
     * @brief Pick the demo options out of an already-parsed command
     * line.
     * @param parsed A command line parsed against a table that included
     * demoFlags().
     * @return The demo options it holds.
     */
    [[nodiscard]] DemoOptions demoOptionsFrom(
        const antwika::cli::CommandLine &parsed);

} // namespace antwika::sound_demo
