#include "antwika/gfx/CubeFace.hpp"

namespace antwika::gfx
{

    Vec3 directionOf(const CubeFace face)
    {
        switch (face)
        {
        case CubeFace::East:
            return Vec3{1.0F, 0.0F, 0.0F};
        case CubeFace::West:
            return Vec3{-1.0F, 0.0F, 0.0F};
        case CubeFace::Up:
            return Vec3{0.0F, 1.0F, 0.0F};
        case CubeFace::Down:
            return Vec3{0.0F, -1.0F, 0.0F};
        case CubeFace::South:
            return Vec3{0.0F, 0.0F, 1.0F};
        case CubeFace::North:
            break;
        }

        return Vec3{0.0F, 0.0F, -1.0F};
    }

    Vec3 upVectorOf(const CubeFace face)
    {
        if (face == CubeFace::Up)
        {
            return Vec3{0.0F, 0.0F, 1.0F};
        }

        if (face == CubeFace::Down)
        {
            return Vec3{0.0F, 0.0F, -1.0F};
        }

        return Vec3{0.0F, -1.0F, 0.0F};
    }

}
