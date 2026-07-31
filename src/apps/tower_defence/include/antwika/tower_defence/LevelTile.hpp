#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace antwika::tower_defence
{

    /**
     * @brief One of the four edges of a square cell.
     *
     * The values are single bits so a tile's open edges are one mask.
     */
    enum class Side : std::uint8_t
    {
        North = 1,
        East = 2,
        South = 4,
        West = 8,
    };

    /** @brief The four sides, in a fixed order every scan uses. */
    inline constexpr std::array<Side, 4> kSides = {
        Side::North, Side::East, Side::South, Side::West};

    /**
     * @brief The tile alphabet the level's wave is solved over.
     *
     * This alphabet is the whole reason a generated path cannot branch.
     * Every tile is open on exactly zero, one or two sides, so no cell
     * can ever have three neighbours -- a T-junction or a crossroads is
     * not a symbol that exists, rather than a shape that is checked for
     * and rejected afterwards.
     * Start and End are the only one-sided tiles, and the generator
     * allows exactly one of each in the whole wave, which is what fixes
     * the two ends of the path.
     *
     * The numbering is load-bearing rather than cosmetic.
     * wfc::Solver tries a cell's candidates in ascending symbol order,
     * so Empty being first makes the solver lay down as little path as
     * the walls force it to.
     * Putting Empty last instead asks for a grid packed with connected
     * tiles, which is a far harder thing to satisfy and measured an
     * order of magnitude slower.
     */
    enum class Tile : std::uint8_t
    {
        Empty = 0,
        NorthSouth = 1,
        EastWest = 2,
        NorthEast = 3,
        SouthEast = 4,
        SouthWest = 5,
        NorthWest = 6,
        Start = 7,
        End = 8,
    };

    /** @brief Size of the tile alphabet, i.e. the wave's symbol count. */
    inline constexpr std::size_t kTileCount = 9;

    /**
     * @brief Get the edges a tile is open on.
     * @param tile The tile to describe.
     * @return A mask of Side bits.
     */
    [[nodiscard]] std::uint8_t openSides(Tile tile);

    /**
     * @brief Check whether a tile is open on one edge.
     * @param tile The tile to test.
     * @param side The edge to test.
     * @return True if the tile connects through that edge.
     */
    [[nodiscard]] bool isOpen(Tile tile, Side side);

    /**
     * @brief Get the edge that faces a given one across a cell border.
     * @param side The edge to reflect.
     * @return North for South, East for West, and so on.
     */
    [[nodiscard]] Side opposite(Side side);

    /**
     * @brief Turn a symbol index back into a tile.
     * @param value A value in [0, kTileCount).
     * @return The tile that symbol stands for.
     */
    [[nodiscard]] Tile tileFromSymbol(std::size_t value);

} // namespace antwika::tower_defence
