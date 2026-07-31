#include "antwika/game/BuildingIndex.hpp"

#include <cstddef>

namespace antwika::game
{

    bool BuildingIndex::insert(Cell cell)
    {
        return occupied.insert(cell).second;
    }

    bool BuildingIndex::erase(Cell cell)
    {
        return occupied.erase(cell) > 0;
    }

    bool BuildingIndex::has(Cell cell) const
    {
        return occupied.contains(cell);
    }

    std::size_t BuildingIndex::size() const noexcept
    {
        return occupied.size();
    }

    const std::set<Cell> &BuildingIndex::cells() const noexcept
    {
        return occupied;
    }

} // namespace antwika::game
