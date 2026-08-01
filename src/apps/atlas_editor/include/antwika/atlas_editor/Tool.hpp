#pragma once

#include <cstddef>
#include <cstdint>

#include <antwika/i18n/MessageId.hpp>

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

        /** @brief Take the pixel's colour as the selected one. */
        Pick,
    };

    /**
     * @brief How many tools there are.
     *
     * Derived from the last enumerator rather than written out, so a new
     * tool is one enumerator and the toolbar grows a button on its own.
     */
    inline constexpr std::size_t kToolCount =
        static_cast<std::size_t>(Tool::Pick) + 1;

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
    [[nodiscard]] antwika::i18n::MessageId toolNameId(Tool tool) noexcept;

} // namespace antwika::atlas_editor
