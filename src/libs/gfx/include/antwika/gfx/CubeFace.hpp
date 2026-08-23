#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "antwika/gfx/Math3D.hpp"

namespace antwika::gfx
{

    enum class CubeFace : std::uint8_t
    {
        East,
        West,
        Up,
        Down,
        South,
        North,
    };

    inline constexpr std::size_t kCubeFaces = 6;

    inline constexpr std::array<CubeFace, kCubeFaces> kEveryCubeFace{
        CubeFace::East,
        CubeFace::West,
        CubeFace::Up,
        CubeFace::Down,
        CubeFace::South,
        CubeFace::North};

    [[nodiscard]] constexpr CubeFace getLastEnumerator(CubeFace) noexcept
    {
        return CubeFace::North;
    }

    [[nodiscard]] Vec3 directionOf(CubeFace face);

    [[nodiscard]] Vec3 upVectorOf(CubeFace face);

}
