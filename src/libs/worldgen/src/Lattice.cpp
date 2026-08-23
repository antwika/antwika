#include "Lattice.hpp"

#include <algorithm>
#include <antwika/worldgen/ruleset/CompiledRuleset.hpp>

namespace antwika::worldgen::detail
{

    namespace
    {
        [[nodiscard]] bool isAnyPartner(
            const wfc::CompatibilityTable &table,
            const wfc::Domain &beyondDomain,
            const std::size_t value,
            const bool asLower)
        {
            for (const std::size_t theirs : beyondDomain)
            {
                const bool fits = asLower ? table.isCompatible(value, theirs)
                                : table.isCompatible(theirs, value);

                if (fits)
                {
                    return true;
                }
            }

            return false;
        }
    }

    std::size_t neighboursOf(
        const ChunkShape shape,
        const std::size_t cell,
        std::array<Neighbour, 6> &foundNeighbours)
    {
        const auto cube = cubeAt(shape, cell);
        const auto width = static_cast<std::size_t>(shape.width);
        const auto depth = static_cast<std::size_t>(shape.depth);

        std::size_t count = 0;

        if (cube.x + 1 < shape.width)
        {
            foundNeighbours[count++] = Neighbour{
                .otherCell = cell + 1, .axis = Axis::Across, .lower = true};
        }

        if (cube.x > 0)
        {
            foundNeighbours[count++] = Neighbour{
                .otherCell = cell - 1, .axis = Axis::Across, .lower = false};
        }

        if (cube.z + 1 < shape.depth)
        {
            foundNeighbours[count++] = Neighbour{
                .otherCell = cell + width, .axis = Axis::Along, .lower = true};
        }

        if (cube.z > 0)
        {
            foundNeighbours[count++] = Neighbour{
                .otherCell = cell - width, .axis = Axis::Along, .lower = false};
        }

        if (cube.y + 1 < shape.height)
        {
            foundNeighbours[count++] = Neighbour{
                .otherCell = cell + (width * depth),
                .axis = Axis::Upright,
                .lower = true};
        }

        if (cube.y > 0)
        {
            foundNeighbours[count++] = Neighbour{
                .otherCell = cell - (width * depth),
                .axis = Axis::Upright,
                .lower = false};
        }

        return count;
    }

    Board::Board(
        std::vector<wfc::Domain> &waveDomains) : heldDomains(&waveDomains)
    {
    }

    const std::vector<wfc::Domain> &Board::getWave() const
    {
        return *heldDomains;
    }

    bool Board::holds(const std::size_t cell, const std::size_t value) const
    {
        return (*heldDomains)[cell].contains(value);
    }

    void Board::take(const std::size_t cell, const std::size_t value)
    {
        (*heldDomains)[cell].remove(value);
        trailCollapses.push_back(Collapse{.cell = cell, .value = value});
    }

    void Board::hold(
        const std::size_t cell, const std::span<const std::size_t> wantedCells)
    {
        const std::vector<std::size_t> domainValues(
            (*heldDomains)[cell].begin(), (*heldDomains)[cell].end());

        for (const std::size_t value : domainValues)
        {
            if (std::ranges::find(wantedCells, value) == wantedCells.end())
            {
                take(cell, value);
            }
        }
    }

    std::size_t Board::getMarkStep() const
    {
        return trailCollapses.size();
    }

    void Board::rewind(const std::size_t toStep)
    {
        while (trailCollapses.size() > toStep)
        {
            const Collapse lastCollapse = trailCollapses.back();
            trailCollapses.pop_back();
            (*heldDomains)[lastCollapse.cell].add(lastCollapse.value);
        }
    }

    bool settle(
        const CompiledRuleset &compiledRuleset,
        const ChunkShape shape,
        Board &board,
        const std::vector<std::size_t> &cells)
    {
        std::vector<std::size_t> worklist = cells;
        std::vector<bool> queuedFlags(getCubeCount(shape), false);

        for (const std::size_t cell : worklist)
        {
            queuedFlags[cell] = true;
        }

        std::size_t head = 0;

        while (head < worklist.size())
        {
            const std::size_t cell = worklist[head];
            ++head;
            queuedFlags[cell] = false;

            std::array<Neighbour, 6> besideNeighbours{};
            const std::size_t count = neighboursOf(
                shape,
                cell,
                besideNeighbours);

            for (std::size_t index = 0; index < count; ++index)
            {
                const Neighbour &sideNeighbour = besideNeighbours[index];
                const auto &table = compiledRuleset.tableAlong(
                    sideNeighbour.axis);

                const std::vector<std::size_t> theirs(
                    board.getWave()[sideNeighbour.otherCell].begin(),
                    board.getWave()[sideNeighbour.otherCell].end());

                bool thinned = false;

                for (const std::size_t value : theirs)
                {
                    if (!isAnyPartner(
                            table,
                            board.getWave()[cell],
                            value,
                            !sideNeighbour.lower))
                    {
                        board.take(sideNeighbour.otherCell, value);
                        thinned = true;
                    }
                }

                if (board.getWave()[sideNeighbour.otherCell].isEmpty())
                {
                    return false;
                }

                if (thinned && !queuedFlags[sideNeighbour.otherCell])
                {
                    queuedFlags[sideNeighbour.otherCell] = true;
                    worklist.push_back(sideNeighbour.otherCell);
                }
            }
        }

        return true;
    }

}
