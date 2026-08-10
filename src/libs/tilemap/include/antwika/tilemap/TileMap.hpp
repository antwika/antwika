#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include <antwika/geometry/Grid.hpp>

#include "antwika/tilemap/Cell.hpp"
#include "antwika/tilemap/Entities.hpp"
#include "antwika/tilemap/MapHeader.hpp"

namespace antwika::tilemap
{

    class TileMap final
    {
    public:
        /**
         * @brief Creates a map of default cells.
         *
         * @param columns Grid width in unit cells; must be at least one.
         * @param rows Grid height in unit cells; must be at least one.
         * @throws TileMapError If columns or rows is zero.
         */
        TileMap(
            MapHeader header,
            std::uint32_t columns,
            std::uint32_t rows);

        [[nodiscard]] const MapHeader &header() const noexcept;
        [[nodiscard]] std::uint32_t columns() const noexcept;
        [[nodiscard]] std::uint32_t rows() const noexcept;

        /**
         * @brief Reaches the cell at the given column and row.
         *
         * @throws TileMapError If the cell lies outside the grid.
         */
        [[nodiscard]] Cell &at(geometry::GridCell cell);

        /**
         * @brief Reads the cell at the given column and row.
         *
         * @throws TileMapError If the cell lies outside the grid.
         */
        [[nodiscard]] const Cell &at(geometry::GridCell cell) const;

        [[nodiscard]] const std::vector<Entity> &entities() const noexcept;

        void addEntity(Entity entity);

    private:
        [[nodiscard]] std::size_t indexOf(geometry::GridCell cell) const;

        MapHeader header_;
        std::uint32_t columns_;
        std::uint32_t rows_;
        std::vector<Cell> cells_{};
        std::vector<Entity> entities_{};
    };

}
