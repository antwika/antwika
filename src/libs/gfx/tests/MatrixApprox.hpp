#pragma once

#include <gtest/gtest.h>

#include <cmath>

#include "antwika/gfx/Math3D.hpp"

namespace antwika::gfx::tests
{

    constexpr float kEpsilon = 1e-5F;

    [[nodiscard]] inline ::testing::AssertionResult approxEqual(
        const Vec4 &left, const Vec4 &right)
    {
        for (int index = 0; index < 4; ++index)
        {
            if (std::fabs(left[index] - right[index]) > kEpsilon)
            {
                return ::testing::AssertionFailure()
                    << "component " << index << ": " << left[index]
                    << " != " << right[index];
            }
        }

        return ::testing::AssertionSuccess();
    }

    [[nodiscard]] inline ::testing::AssertionResult approxEqual(
        const Vec3 &left, const Vec3 &right)
    {
        return approxEqual(Vec4(left, 0.0F), Vec4(right, 0.0F));
    }

    [[nodiscard]] inline ::testing::AssertionResult approxEqual(
        const Mat4 &left, const Mat4 &right)
    {
        for (int column = 0; column < 4; ++column)
        {
            const auto result = approxEqual(left[column], right[column]);

            if (!result)
            {
                return ::testing::AssertionFailure()
                    << "column " << column << ": " << result.message();
            }
        }

        return ::testing::AssertionSuccess();
    }

}
