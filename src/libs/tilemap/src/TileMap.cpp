#include "antwika/tilemap/TileMap.hpp"

#include <cstddef>
#include <utility>

#include "antwika/tilemap/Slab.hpp"
#include "antwika/tilemap/TileMapError.hpp"

namespace antwika::tilemap
{

    TileMap::TileMap(
        MapHeader header,
        const std::uint32_t columns,
        const std::uint32_t rows)
        : header_(std::move(header)),
          columns_(columns),
          rows_(rows)
    {
        if (columns == 0 || rows == 0)
        {
            throw TileMapError("a tile map needs at least one cell");
        }

        grid_.resize(
            static_cast<std::size_t>(columns)
            * static_cast<std::size_t>(rows));

        for (auto &column : grid_)
        {
            (void)column.place(Slab{});
        }
    }

    const MapHeader &TileMap::header() const noexcept
    {
        return header_;
    }

    std::uint32_t TileMap::columns() const noexcept
    {
        return columns_;
    }

    std::uint32_t TileMap::rows() const noexcept
    {
        return rows_;
    }

    Column &TileMap::at(const geometry::GridCell cell)
    {
        return grid_[indexOf(cell)];
    }

    const Column &TileMap::at(const geometry::GridCell cell) const
    {
        return grid_[indexOf(cell)];
    }

    const std::vector<Entity> &TileMap::entities() const noexcept
    {
        return entities_;
    }

    void TileMap::addEntity(Entity entity)
    {
        entities_.push_back(std::move(entity));
    }

    std::size_t TileMap::indexOf(const geometry::GridCell cell) const
    {
        if (cell.column >= columns_ || cell.row >= rows_)
        {
            throw TileMapError("the cell lies outside the map");
        }

        return static_cast<std::size_t>(cell.row) * columns_
               + cell.column;
    }

}
