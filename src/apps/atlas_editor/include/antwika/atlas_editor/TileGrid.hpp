#pragma once

#include <cstdint>
#include <optional>

#include <antwika/gfx/Size.hpp>

#include "antwika/atlas_editor/Pixel.hpp"

namespace antwika::atlas_editor
{

    using antwika::gfx::Size;

    /**
     * @brief How wide one tile of the game's sheet is, in pixels.
     *
     * The default this editor grids an atlas with, and the one
     * `src/apps/game/assets/atlas.png` is drawn to: see
     * docs/game-texture-atlas.md, which is the contract the art has to
     * meet.
     * It is a default rather than a constant baked into the code,
     * because a sheet for something else is grided with `--tile`.
     */
    inline constexpr std::uint32_t kDefaultTileWidth = 128;

    /**
     * @brief How tall one tile of the game's sheet is, in pixels.
     */
    inline constexpr std::uint32_t kDefaultTileHeight = 64;

    /**
     * @brief How an image is divided into the slots an artist paints.
     *
     * The grid is drawn over the image and nothing else: no tool, no
     * click and no saved byte depends on it, so getting it wrong shows a
     * misleading picture and cannot corrupt a sheet.
     */
    struct TileGrid
    {
        std::uint32_t width = kDefaultTileWidth;
        std::uint32_t height = kDefaultTileHeight;

        /**
         * @brief Compare two grids.
         * @param other The grid to compare against.
         * @return True when both dimensions match.
         */
        [[nodiscard]] bool operator==(const TileGrid &other) const =
            default;
    };

    /**
     * @brief How many columns of slots an image of this size holds.
     *
     * Whole slots only: a sheet whose width is not a multiple of the
     * tile width has a strip along its right edge that belongs to no
     * slot, and saying it were a column would put a slot number on
     * pixels the game will never blit.
     *
     * @param grid How the image is divided.
     * @param image How big the image is.
     * @return The count, which is zero for a grid or an image with no
     * extent at all.
     */
    [[nodiscard]] std::uint32_t columnsIn(
        TileGrid grid, Size image) noexcept;

    /**
     * @brief How many rows of slots an image of this size holds.
     * @param grid How the image is divided.
     * @param image How big the image is.
     * @return The count, on columnsIn()'s terms.
     */
    [[nodiscard]] std::uint32_t rowsIn(TileGrid grid, Size image) noexcept;

    /**
     * @brief Work out which slot a pixel belongs to.
     *
     * Counted left to right then top to bottom from slot zero, which is
     * how game::TileAtlas.hpp addresses the sheet this editor exists to
     * serve -- so the number shown here is the number that header names.
     *
     * @param grid How the image is divided.
     * @param image How big the image is.
     * @param pixel The pixel to place.
     * @return Its slot, or nothing when the pixel is outside the image
     * or in a partial slot along an edge.
     */
    [[nodiscard]] std::optional<std::uint32_t> slotAt(
        TileGrid grid, Size image, Pixel pixel) noexcept;

} // namespace antwika::atlas_editor
