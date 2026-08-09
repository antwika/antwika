#pragma once

#include <optional>

#include <antwika/atlas/AtlasMeta.hpp>

#include "antwika/atlas_editor/SpriteGuides.hpp"
#include "antwika/atlas_editor/TileGrid.hpp"

namespace antwika::atlas_editor
{

    using antwika::atlas::AtlasKind;
    using antwika::atlas::AtlasMeta;
    using antwika::atlas::counted;
    using antwika::atlas::kAtlasKindCount;
    using antwika::atlas::sheetSizeOf;

    /**
     * @brief Describes the atlas a slot size and a sheet stand for.
     *
     * @param tile The slot the sheet is cut into.
     * @param sheet The sheet those slots are cut from.
     * @return The metadata, isometric where the slot carries a
     *         footprint and flat where it does not.
     */
    [[nodiscard]] AtlasMeta metaFor(TileGrid tile, Size sheet) noexcept;

    [[nodiscard]] TileGrid tilesOf(const AtlasMeta &meta) noexcept;

    /**
     * @brief Places the guides an atlas asks for inside one slot.
     *
     * @param meta The atlas to read.
     * @return The guides, or nothing for an atlas that is not
     *         isometric or whose footprint has no extent.
     */
    [[nodiscard]] std::optional<SpriteGuides> guidesOf(
        const AtlasMeta &meta) noexcept;

}
