#include "antwika/game/WorldMap.hpp"

#include <algorithm>
#include <cstdint>
#include <functional>

#include <antwika/rng/SplitMix64Rng.hpp>
#include <antwika/wfc/AdjacencyConstraint.hpp>
#include <antwika/wfc/CompatibilityTable.hpp>
#include <antwika/wfc/IConstraint.hpp>
#include <antwika/wfc/SolveResult.hpp>
#include <antwika/wfc/Solver.hpp>

#include "antwika/game/WorldMapError.hpp"

namespace antwika::game
{

    using antwika::rng::SplitMix64Rng;
    using antwika::wfc::AdjacencyConstraint;
    using antwika::wfc::CompatibilityTable;
    using antwika::wfc::Domain;
    using antwika::wfc::IConstraint;
    using antwika::wfc::SolveOutcome;
    using antwika::wfc::SolveResult;
    using antwika::wfc::Solver;

    namespace
    {
        // How far apart the seeded anchors sit, in cells.
        // Also how many ladder steps two of them may differ by.
        // That is the same number for a reason.
        // A partial assignment on a grid extends to a whole one.
        // Only when no pinned pair differs by more than its gap.
        // So pinning at distance N and within N is always solvable.
        constexpr std::uint32_t kAnchorSpacing = 2;
        constexpr std::int64_t kAnchorReach = kAnchorSpacing;

        constexpr std::uint32_t kMinimumSide = 4;

        // The one adjacency rule.
        // Neighbours differ by at most one ladder step.
        CompatibilityTable ladderTable()
        {
            CompatibilityTable table(kTerrainCount);
            for (std::size_t left = 0; left < kTerrainCount; ++left)
            {
                for (std::size_t right = 0; right < kTerrainCount;
                     ++right)
                {
                    const std::size_t low = std::min(left, right);
                    const std::size_t high = std::max(left, right);
                    table.set(left, right, high - low <= 1);
                }
            }
            return table;
        } // GCOVR_EXCL_LINE

        std::vector<AdjacencyConstraint> ladderConstraints(
            std::uint32_t width, std::uint32_t height)
        {
            const CompatibilityTable table = ladderTable();
            std::vector<AdjacencyConstraint> constraints;
            for (std::uint32_t y = 0; y < height; ++y)
            {
                for (std::uint32_t x = 0; x < width; ++x)
                {
                    const std::size_t here =
                        static_cast<std::size_t>(y) * width + x;
                    if (x + 1 < width)
                    {
                        constraints.emplace_back(
                            here, here + 1, table);
                    }
                    if (y + 1 < height)
                    {
                        constraints.emplace_back(
                            here, here + width, table);
                    }
                }
            }
            return constraints;
        } // GCOVR_EXCL_LINE

        // How good a land cell is to build a city on.
        // Land neighbours first, then nearness to plains.
        // Both are small integers.
        // So no ordering here can depend on a float comparison.
        struct Candidate
        {
            bool has = false;
            std::size_t index = 0;
            std::size_t neighbours = 0;
            std::size_t rank = 0;
        };

        bool better(const Candidate &lhs, const Candidate &rhs)
        {
            if (!rhs.has)
            {
                return true;
            }
            if (lhs.neighbours != rhs.neighbours)
            {
                return lhs.neighbours > rhs.neighbours;
            }
            return lhs.rank < rhs.rank;
        }

        std::size_t landNeighbours(
            std::uint32_t width,
            std::uint32_t height,
            const std::vector<Terrain> &tiles,
            std::uint32_t x,
            std::uint32_t y)
        {
            const std::size_t row = static_cast<std::size_t>(y) * width;
            std::size_t count = 0;
            if (x > 0 && isLand(tiles[row + x - 1]))
            {
                ++count;
            }
            if (x + 1 < width && isLand(tiles[row + x + 1]))
            {
                ++count;
            }
            if (y > 0 && isLand(tiles[row - width + x]))
            {
                ++count;
            }
            if (y + 1 < height && isLand(tiles[row + width + x]))
            {
                ++count;
            }
            return count;
        }

        // Distance from plains along the ladder.
        // So plains beats forest beats hills beats mountain.
        std::size_t plainsRank(Terrain terrain)
        {
            const std::size_t symbol = symbolOf(terrain);
            const std::size_t plains = symbolOf(Terrain::Plains);
            return symbol > plains ? symbol - plains : plains - symbol;
        }

        std::size_t quadrantOf(
            std::uint32_t width,
            std::uint32_t height,
            std::uint32_t x,
            std::uint32_t y)
        {
            const std::size_t column = x < width / 2 ? 0U : 1U;
            const std::size_t row = y < height / 2 ? 0U : 1U;
            return row * 2 + column;
        }
    } // namespace

    Terrain WorldMap::at(std::uint32_t x, std::uint32_t y) const
    {
        if (x >= width || y >= height)
        {
            throw WorldMapError("World map coordinate is off the map");
        }
        return tiles[static_cast<std::size_t>(y) * width + x];
    }

    Cell WorldMap::cityCell(std::size_t city) const
    {
        if (city >= kCityCount)
        {
            throw WorldMapError("No such city on this world map");
        }
        const std::size_t index = cities[city];
        return Cell{
            static_cast<std::int32_t>(index % width),
            static_cast<std::int32_t>(index / width)};
    }

    std::size_t WorldMap::cityAt(Cell cell) const
    {
        for (std::size_t city = 0; city < kCityCount; ++city)
        {
            if (cityCell(city) == cell)
            {
                return city;
            }
        }
        return kCityCount;
    }

    std::vector<Domain> buildWorldWave(const WorldMapConfig &config)
    {
        const std::size_t cellCount =
            static_cast<std::size_t>(config.width) * config.height;
        std::vector<Domain> wave(cellCount, Domain(kTerrainCount));

        const std::uint32_t latticeWidth =
            (config.width + kAnchorSpacing - 1) / kAnchorSpacing;
        const std::uint32_t latticeHeight =
            (config.height + kAnchorSpacing - 1) / kAnchorSpacing;

        std::vector<std::int64_t> anchors(
            static_cast<std::size_t>(latticeWidth) * latticeHeight, 0);
        SplitMix64Rng rng(config.seed);

        for (std::uint32_t ly = 0; ly < latticeHeight; ++ly)
        {
            for (std::uint32_t lx = 0; lx < latticeWidth; ++lx)
            {
                const std::size_t slot =
                    static_cast<std::size_t>(ly) * latticeWidth + lx;
                std::int64_t low = 0;
                std::int64_t high =
                    static_cast<std::int64_t>(kTerrainCount) - 1;
                if (lx > 0)
                {
                    const std::int64_t left = anchors[slot - 1];
                    low = std::max(low, left - kAnchorReach);
                    high = std::min(high, left + kAnchorReach);
                }
                if (ly > 0)
                {
                    const std::int64_t up =
                        anchors[slot - latticeWidth];
                    low = std::max(low, up - kAnchorReach);
                    high = std::min(high, up + kAnchorReach);
                }

                const std::uint64_t span =
                    static_cast<std::uint64_t>(high - low) + 1;
                const std::int64_t value = low
                    + static_cast<std::int64_t>(rng.next() % span);
                anchors[slot] = value;

                const std::size_t cell =
                    static_cast<std::size_t>(ly) * kAnchorSpacing
                        * config.width
                    + static_cast<std::size_t>(lx) * kAnchorSpacing;
                wave[cell] = Domain::singleton(
                    static_cast<std::size_t>(value), kTerrainCount);
            }
        }

        return wave;
    } // GCOVR_EXCL_LINE

    std::vector<Terrain> solveTerrain(
        std::uint32_t width,
        std::uint32_t height,
        std::vector<Domain> wave)
    {
        const std::size_t cellCount =
            static_cast<std::size_t>(width) * height;
        if (wave.size() != cellCount)
        {
            throw WorldMapError(
                "World map wave does not match the map's size");
        }

        const std::vector<AdjacencyConstraint> constraints =
            ladderConstraints(width, height);
        std::vector<std::reference_wrapper<const IConstraint>> refs;
        refs.reserve(constraints.size());
        for (const AdjacencyConstraint &constraint : constraints)
        {
            refs.emplace_back(constraint);
        }

        const Solver solver(std::move(wave), std::move(refs));
        const SolveResult result = solver.solve();
        if (result.outcome != SolveOutcome::Solved)
        {
            throw WorldMapError(
                "No terrain satisfies the world map's adjacency rule");
        }

        std::vector<Terrain> tiles;
        tiles.reserve(cellCount);
        for (const std::size_t symbol : result.assignment)
        {
            tiles.push_back(terrainOf(symbol));
        }
        return tiles;
    } // GCOVR_EXCL_LINE

    std::array<std::size_t, kCityCount> placeCities(
        std::uint32_t width,
        std::uint32_t height,
        const std::vector<Terrain> &tiles)
    {
        const std::size_t cellCount =
            static_cast<std::size_t>(width) * height;
        if (tiles.size() != cellCount)
        {
            throw WorldMapError(
                "World map tiles do not match the map's size");
        }

        std::array<Candidate, kCityCount> best{};
        std::size_t landCells = 0;
        for (std::uint32_t y = 0; y < height; ++y)
        {
            for (std::uint32_t x = 0; x < width; ++x)
            {
                const std::size_t index =
                    static_cast<std::size_t>(y) * width + x;
                if (!isLand(tiles[index]))
                {
                    continue;
                }
                ++landCells;
                const Candidate here{
                    true,
                    index,
                    landNeighbours(width, height, tiles, x, y),
                    plainsRank(tiles[index])};
                const std::size_t quadrant =
                    quadrantOf(width, height, x, y);
                if (better(here, best[quadrant]))
                {
                    best[quadrant] = here;
                }
            }
        }

        if (landCells < kCityCount)
        {
            throw WorldMapError(
                "World map has too little land to seat four cities");
        }

        // A quadrant that came up all water still owes a city.
        // It takes the best land cell nobody has claimed.
        // The land count above is what makes this find one.
        for (Candidate &slot : best)
        {
            if (slot.has)
            {
                continue;
            }
            for (std::uint32_t y = 0; y < height; ++y)
            {
                for (std::uint32_t x = 0; x < width; ++x)
                {
                    const std::size_t index =
                        static_cast<std::size_t>(y) * width + x;
                    if (!isLand(tiles[index]))
                    {
                        continue;
                    }
                    const bool taken = std::ranges::any_of(
                        best,
                        [index](const Candidate &other)
                        {
                            return other.has && other.index == index;
                        });
                    if (taken)
                    {
                        continue;
                    }
                    const Candidate here{
                        true,
                        index,
                        landNeighbours(width, height, tiles, x, y),
                        plainsRank(tiles[index])};
                    if (better(here, slot))
                    {
                        slot = here;
                    }
                }
            }
        }

        std::array<std::size_t, kCityCount> cities{};
        for (std::size_t i = 0; i < kCityCount; ++i)
        {
            cities[i] = best[i].index;
        }
        std::ranges::sort(cities);
        return cities;
    } // GCOVR_EXCL_LINE

    WorldMap generateWorldMap(const WorldMapConfig &config)
    {
        if (config.width < kMinimumSide || config.height < kMinimumSide)
        {
            throw WorldMapError(
                "A world map must be at least four cells on a side");
        }

        WorldMap map;
        map.width = config.width;
        map.height = config.height;
        map.tiles = solveTerrain(
            config.width, config.height, buildWorldWave(config));
        map.cities =
            placeCities(config.width, config.height, map.tiles);
        return map;
    } // GCOVR_EXCL_LINE

} // namespace antwika::game
