#include "antwika/tower_defence/LevelTile.hpp"

namespace antwika::tower_defence
{

    namespace
    {
        // Indexed by the Tile enumerator's own value.
        // Start opens east only and End opens west only.
        // That makes them the only degree-one tiles in the alphabet.
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
    } // namespace

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
        // North and South are two bits apart, as are East and West.
        // A rotation by two in the low nibble is the reflection.
        const auto bits = static_cast<std::uint8_t>(side);
        const auto rotated =
            static_cast<std::uint8_t>(((bits << 2) | (bits >> 2)) & 0x0F);
        return static_cast<Side>(rotated);
    }

    Tile tileFromSymbol(const std::size_t value)
    {
        return static_cast<Tile>(value);
    }

} // namespace antwika::tower_defence
