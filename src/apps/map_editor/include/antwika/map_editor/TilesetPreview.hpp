#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <antwika/tileset/Tileset.hpp>

namespace antwika::map_editor
{

    inline constexpr std::int32_t kPreviewColumns = 11;

    inline constexpr std::int32_t kPreviewRows = 5;

    inline constexpr std::int32_t kPreviewCenterColumn = 5;

    inline constexpr std::int32_t kPreviewCenterRow = 2;

    inline constexpr std::size_t kPreviewCells =
        static_cast<std::size_t>(kPreviewColumns)
        * static_cast<std::size_t>(kPreviewRows);

    struct TilesetPreview final
    {
        std::array<bool, kPreviewCells> outside{};

        /**
         * @brief Layer-0 sprite index per cell, or -1 outside the
         *        region.
         */
        std::array<std::int32_t, kPreviewCells> base{};

        /**
         * @brief Decor sprite index per cell for each layer at index
         *        1 and up, or -1 where the layer places nothing.
         */
        std::vector<std::array<std::int32_t, kPreviewCells>> decor{};

        bool centerBaseMissing = false;
    };

    /**
     * @brief Generates the combination the preview panel shows.
     *
     * @param data The tileset the preview assembles sprites from.
     * @param layer The selected sprite's layer index.
     * @param sprite The selected sprite's index in that layer.
     * @param seed The arrangement seed; equal inputs give equal
     *        output.
     * @return The generated cells, with the selected sprite pinned at
     *         the center of its layer.
     *
     * Ensures: an edge socket on the selected sprite marks every cell
     *          strictly beyond that side of the center as outside,
     *          every inside cell holds a base sprite, and a decor
     *          selection whose allowlist names no base sprite sets
     *          centerBaseMissing and pins nothing.
     * Ensures: wherever several sprites remain valid at a cell, the
     *          pick among them is weighted by sprite weight, matching
     *          the map assembler.
     */
    [[nodiscard]] TilesetPreview buildTilesetPreview(
        const tileset::Tileset &data,
        std::size_t layer,
        std::size_t sprite,
        std::uint32_t seed);

}
