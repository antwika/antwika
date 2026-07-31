#pragma once

#include <optional>
#include <span>
#include <string>
#include <string_view>

#include <antwika/gfx/Size.hpp>
#include <antwika/replay/CommandLine.hpp>
#include <antwika/replay/FlagSpec.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/atlas_editor/AtlasEditor.hpp"
#include "antwika/atlas_editor/TileGrid.hpp"

namespace antwika::atlas_editor
{

    using antwika::gfx::Size;

    /**
     * @brief How long a session runs when nobody said.
     *
     * **An editor has no end of its own**, so this is what keeps one
     * from running forever: an hour at the frame period the application
     * paces itself with.
     * A real backend reports the window closing and Escape, and either
     * ends a session long before this; the default `null` backend
     * reports neither, and every CI leg builds that one -- so an
     * uncapped run there is a run that never finishes.
     * `--max-ticks 0` asks for exactly that, for somebody sitting in
     * front of a real window who does not want their afternoon cut
     * short.
     */
    inline constexpr antwika::time::Tick kDefaultMaxTicks = 90000;

    /**
     * @brief What this application was asked to edit, and how.
     */
    struct EditorOptions
    {
        /**
         * @brief The PNG to open, if any.
         *
         * Nothing means starting on a blank sheet of `sheet` pixels.
         */
        std::optional<std::string> imagePath{};

        /**
         * @brief The PNG a save writes, if any.
         *
         * Deliberately not defaulted to imagePath: the sheet somebody is
         * most likely to open first is the game's own, and one stray
         * click should not be able to overwrite the art with it.
         * Naming this is how an artist says which file is theirs.
         */
        std::optional<std::string> outPath{};

        /** @brief How big a blank sheet to open. */
        Size sheet = kDefaultSheetSize;

        /** @brief How to divide the sheet for the grid overlay. */
        TileGrid tile{};

        /**
         * @brief How many ticks to run at most, or nothing for no cap.
         */
        std::optional<antwika::time::Tick> maxTicks{kDefaultMaxTicks};

        /**
         * @brief Compare two sets of options.
         * @param other The options to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(const EditorOptions &other) const =
            default;
    };

    /**
     * @brief Read a `<width>x<height>` pair.
     *
     * Its own function because two flags take one, and because what a
     * malformed one means is a decision worth making once.
     *
     * @param text The value as it was typed.
     * @return The size, or nothing when the text is not two positive
     * numbers with an `x` between them.
     */
    [[nodiscard]] std::optional<Size> parseSize(std::string_view text);

    /**
     * @brief The flags this application accepts on top of the replay
     * ones.
     *
     * Handed to antwika::app::runRecorded(), which parses them in the
     * same pass as its own -- parsing them in a pass of their own is
     * what once stopped apps/poker's `--tick-delay-ms` working, since
     * the other pass had already refused it.
     *
     * @return The table, for a main() to pass on.
     */
    [[nodiscard]] std::span<const antwika::replay::FlagSpec>
    editorFlags();

    /**
     * @brief Read this application's options out of a parsed command
     * line.
     *
     * A value that is not what a flag asks for is ignored and the
     * default kept, following apps/poker's `--tick-delay-ms`: a typo in
     * a window size should not stop somebody editing.
     *
     * @param parsed A command line parsed against a table that included
     * editorFlags().
     * @return What was asked for.
     */
    [[nodiscard]] EditorOptions editorOptionsFrom(
        const antwika::replay::CommandLine &parsed);

} // namespace antwika::atlas_editor
