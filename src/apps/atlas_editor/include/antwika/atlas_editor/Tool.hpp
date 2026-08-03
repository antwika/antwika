#pragma once

#include <cstddef>
#include <cstdint>

#include "antwika/atlas_editor/MessageId.hpp"

namespace antwika::atlas_editor
{

    /**
     * @brief What a left click on the image does.
     */
    enum class Tool : std::uint8_t
    {
        /** @brief Put the selected colour into the pixel. */
        Paint = 0,

        /** @brief Make the pixel fully transparent. */
        Erase,

        /**
         * @brief Spread the selected colour over every pixel joined to
         * this one that holds the colour this one does.
         *
         * Four-connected and bounded by the sheet, so the same click
         * fills the same pixels on every run -- which is what lets it be
         * a tool at all here, since nothing about a fill is persisted
         * and a replay works the whole region out again from the press.
         */
        Fill,

        /** @brief Take the pixel's colour as the selected one. */
        Pick,

        /**
         * @brief Mark a rectangle of the sheet out, and move it.
         *
         * The one tool whose left button is a gesture rather than a
         * brush: a drag from outside the marked rectangle draws a new
         * one, and a drag from inside it carries its pixels somewhere
         * else when the button comes up.
         */
        Select,
    };

    /**
     * @brief How many tools there are.
     *
     * Derived from the last enumerator rather than written out, so a new
     * tool is one enumerator and the toolbar grows a button on its own.
     */
    inline constexpr std::size_t kToolCount =
        static_cast<std::size_t>(Tool::Select) + 1;

    /**
     * @brief Get which message names a tool.
     *
     * An id rather than the words, so the toolbar and the status line
     * are worded by whatever holds the translator and this file holds
     * no language at all.
     *
     * @param tool The tool to name.
     * @return Its message id.
     */
    [[nodiscard]] MessageId toolNameId(Tool tool) noexcept;

} // namespace antwika::atlas_editor
