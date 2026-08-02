#pragma once

#include <cstddef>
#include <optional>
#include <string_view>

#include "antwika/ui/Keyboard.hpp"
#include "antwika/ui/TextEdit.hpp"
#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui::detail
{

    /**
     * @brief What a frame's typing is being applied to.
     */
    struct Editable
    {
        /** @brief Which widget the edit will be reported against. */
        WidgetId id = kNoWidget;

        /** @brief The characters as they now stand. */
        std::string_view text{};

        /** @brief The caret, already brought inside the text. */
        std::size_t cursor = 0;

        /**
         * @brief The selection's other end, brought inside the text.
         *
         * Equal to the caret for a field with nothing selected, which
         * is what makes "take the selection" and "take one character"
         * one branch rather than two paths through this whole file.
         */
        std::size_t anchor = 0;

        /**
         * @brief Whether Activate puts a line break in.
         *
         * A field has one line and submits on it; an area is where a
         * line break is written, and has nothing to submit to.
         */
        bool multiline = false;
    };

    /**
     * @brief Get where the line holding an index starts.
     * @param text The characters to look in.
     * @param at An index inside them, or one past the end.
     * @return The index of the first character of that line.
     */
    [[nodiscard]] std::size_t beginOfLine(
        std::string_view text, std::size_t at) noexcept;

    /**
     * @brief Get where the line holding an index ends.
     * @param text The characters to look in.
     * @param at An index inside them, or one past the end.
     * @return The index of that line's break, or the end of the text.
     */
    [[nodiscard]] std::size_t endOfLine(
        std::string_view text, std::size_t at) noexcept;

    /**
     * @brief Work out what this frame's typing came to.
     *
     * Applied to a copy of the caller's characters, never to the
     * caller's own: this library holds nothing between frames, so what
     * it can offer is the answer rather than the edit.
     *
     * **Every edge is read in the order it arrived**, characters
     * included, which is why a character arrives as a Key::Character
     * edge indexing into Keyboard::typed rather than as a lump applied
     * before the keys.
     *
     * @param field What is being typed into.
     * @param keys What arrived this frame.
     * @return The edit, or nothing when this frame left the characters
     * and the caret exactly as they were.
     */
    [[nodiscard]] std::optional<TextEdit> editFor(
        const Editable &field, const Keyboard &keys);

} // namespace antwika::ui::detail
