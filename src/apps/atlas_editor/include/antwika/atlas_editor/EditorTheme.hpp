#pragma once

#include <cstddef>

#include <antwika/gfx/Size.hpp>
#include <antwika/ui/Theme.hpp>

#include "antwika/atlas_editor/Modal.hpp"

namespace antwika::atlas_editor
{

    using antwika::gfx::Size;

    inline constexpr std::size_t kCardLabels = 2;

    inline constexpr std::size_t kMetaLines = 5;

    [[nodiscard]] antwika::ui::Theme editorTheme(Size canvas) noexcept;

    /**
     * @brief Counts the label lines a modal writes above its listing.
     *
     * @param modal The modal being drawn.
     * @return The lines it spends before the first file row.
     */
    [[nodiscard]] std::size_t labelsAbove(Modal modal) noexcept;

    /**
     * @brief Counts the file rows the load and save card has room for.
     *
     * @param canvas The canvas the card is drawn on.
     * @param labels The label lines written above the listing.
     * @return The rows that fit, never fewer than one.
     *
     * Ensures: a card holding that many rows is no taller than the
     *          canvas, so nothing on it is squeezed out of sight.
     */
    [[nodiscard]] std::size_t filesShownIn(
        Size canvas, std::size_t labels) noexcept;

}
