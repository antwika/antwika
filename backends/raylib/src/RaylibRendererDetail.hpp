#pragma once

#include <raylib.h>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Math3D.hpp>

namespace antwika::gfx::raylib
{

    inline ::Color toRaylib(Color color)
    {
        return ::Color{
            .r = color.red,
            .g = color.green,
            .b = color.blue,
            .a = color.alpha};
    }

    inline ::Matrix toRaylib(const Mat4 &matrix)
    {
        return ::Matrix{
            .m0 = matrix[0][0],
            .m4 = matrix[1][0],
            .m8 = matrix[2][0],
            .m12 = matrix[3][0],
            .m1 = matrix[0][1],
            .m5 = matrix[1][1],
            .m9 = matrix[2][1],
            .m13 = matrix[3][1],
            .m2 = matrix[0][2],
            .m6 = matrix[1][2],
            .m10 = matrix[2][2],
            .m14 = matrix[3][2],
            .m3 = matrix[0][3],
            .m7 = matrix[1][3],
            .m11 = matrix[2][3],
            .m15 = matrix[3][3]};
    }

}
