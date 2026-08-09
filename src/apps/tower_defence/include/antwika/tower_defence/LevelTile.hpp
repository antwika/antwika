#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace antwika::tower_defence
{

    enum class Side : std::uint8_t
    {
        North = 1,
        East = 2,
        South = 4,
        West = 8,
    };

    inline constexpr std::array<Side, 4> kSides = {
        Side::North, Side::East, Side::South, Side::West};

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

    inline constexpr std::size_t kTileCount = 9;

    [[nodiscard]] std::uint8_t openSides(Tile tile);

    [[nodiscard]] bool isOpen(Tile tile, Side side);

    [[nodiscard]] Side opposite(Side side);

    [[nodiscard]] Tile tileFromSymbol(std::size_t value);

}
