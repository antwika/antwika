#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include <antwika/geometry/Grid.hpp>

#include "antwika/tilemap/Column.hpp"
#include "antwika/tilemap/Entities.hpp"
#include "antwika/tilemap/MapHeader.hpp"

namespace antwika::tilemap
{

    class TileMap final
    {
    public:
        /**
         * @brief Creates a map of single-slab columns.
         *
         * @param columns Grid width in unit cells; must be at least one.
         * @param rows Grid height in unit cells; must be at least one.
         * @throws TileMapError If columns or rows is zero.
         *
         * Ensures: every column holds a single default floor slab at
         *          level zero.
         */
        TileMap(
            MapHeader header,
            std::uint32_t columns,
            std::uint32_t rows);

        [[nodiscard]] const MapHeader &header() const noexcept;
        [[nodiscard]] std::uint32_t columns() const noexcept;
        [[nodiscard]] std::uint32_t rows() const noexcept;

        /**
         * @brief Reaches the column at the given cell.
         *
         * @throws TileMapError If the cell lies outside the grid.
         */
        [[nodiscard]] Column &at(geometry::GridCell cell);

        /**
         * @brief Reads the column at the given cell.
         *
         * @throws TileMapError If the cell lies outside the grid.
         */
        [[nodiscard]] const Column &at(geometry::GridCell cell) const;

        [[nodiscard]] const std::vector<Entity> &entities() const noexcept;

        void addEntity(Entity entity);

    private:
        [[nodiscard]] std::size_t indexOf(geometry::GridCell cell) const;

        MapHeader header_;
        std::uint32_t columns_;
        std::uint32_t rows_;
        std::vector<Column> grid_{};
        std::vector<Entity> entities_{};
    };

}
