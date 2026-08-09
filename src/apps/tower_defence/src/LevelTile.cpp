#include "antwika/tower_defence/LevelTile.hpp"

namespace antwika::tower_defence
{

    namespace
    {
        constexpr std::array<std::uint8_t, kTileCount> kOpenSides = {
            0,
            static_cast<std::uint8_t>(Side::North)
                | static_cast<std::uint8_t>(Side::South),
            static_cast<std::uint8_t>(Side::East)
                | static_cast<std::uint8_t>(Side::West),
            static_cast<std::uint8_t>(Side::North)
                | static_cast<std::uint8_t>(Side::East),
            static_cast<std::uint8_t>(Side::South)
                | static_cast<std::uint8_t>(Side::East),
            static_cast<std::uint8_t>(Side::South)
                | static_cast<std::uint8_t>(Side::West),
            static_cast<std::uint8_t>(Side::North)
                | static_cast<std::uint8_t>(Side::West),
            static_cast<std::uint8_t>(Side::East),
            static_cast<std::uint8_t>(Side::West),
        };
    }

    std::uint8_t openSides(const Tile tile)
    {
        return kOpenSides[static_cast<std::size_t>(tile)];
    }

    bool isOpen(const Tile tile, const Side side)
    {
        return (openSides(tile) & static_cast<std::uint8_t>(side)) != 0;
    }

    Side opposite(const Side side)
    {
        const auto bits = static_cast<std::uint8_t>(side);
        const auto rotated =
            static_cast<std::uint8_t>(((bits << 2) | (bits >> 2)) & 0x0F);
        return static_cast<Side>(rotated);
    }

    Tile tileFromSymbol(const std::size_t value)
    {
        return static_cast<Tile>(value);
    }

}
