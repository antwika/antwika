#pragma once

#include <span>

#include <antwika/gfx/Color.hpp>

namespace antwika::atlas_editor
{

    using antwika::gfx::Color;

    /**
     * @brief The colours the toolbar offers, in the order they are drawn.
     *
     * Compiled in rather than loaded, exactly as antwika::i18n's
     * catalogues are: this application opens the one file it was asked
     * to edit and no others.
     *
     * They are the values the game's own sheet is drawn in -- ground,
     * road, roof, water and the greys between them -- since an artist
     * repainting one tile wants the tile beside it to still match.
     * Every one of them is fully opaque: a transparent pixel is what the
     * Erase tool leaves, so a clear swatch would be a second way to say
     * one thing.
     *
     * @return The palette, which is never empty and never changes.
     */
    [[nodiscard]] std::span<const Color> defaultPalette() noexcept;

} // namespace antwika::atlas_editor
