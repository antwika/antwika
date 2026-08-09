#include "antwika/game/PathIndex.hpp"

#include "antwika/game/Direction.hpp"

namespace antwika::game
{

    bool PathIndex::insert(Cell cell)
    {
        return paths.insert(cell).second;
    }

    bool PathIndex::erase(Cell cell)
    {
        return paths.erase(cell) > 0;
    }

    bool PathIndex::has(Cell cell) const
    {
        return paths.contains(cell);
    }

    Neighbours PathIndex::neighboursOf(Cell cell) const
    {
        return Neighbours{
            .north = has(step(cell, Direction::North)),
            .east = has(step(cell, Direction::East)),
            .south = has(step(cell, Direction::South)),
            .west = has(step(cell, Direction::West))};
    }

    std::size_t PathIndex::size() const noexcept
    {
        return paths.size();
    }

    const std::set<Cell> &PathIndex::cells() const noexcept
    {
        return paths;
    }

}
