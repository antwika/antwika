#pragma once

#include <gtest/gtest.h>

#include <cmath>

#include "antwika/gfx/Math3D.hpp"

namespace antwika::gfx::tests
{

    constexpr float kEpsilon = 1e-5F;

    [[nodiscard]] inline ::testing::AssertionResult getApproxEqual(
        const Vec4 &leftVector, const Vec4 &rightVector)
    {
        for (int index = 0; index < 4; ++index)
        {
            if (std::fabs(leftVector[index] - rightVector[index]) > kEpsilon)
            {
                return ::testing::AssertionFailure()
                    << "component " << index << ": " << leftVector[index]
                    << " != " << rightVector[index];
            }
        }

        return ::testing::AssertionSuccess();
    }

    [[nodiscard]] inline ::testing::AssertionResult getApproxEqual(
        const Vec3 &leftVector, const Vec3 &rightVector)
    {
        return getApproxEqual(Vec4(leftVector, 0.0F), Vec4(rightVector, 0.0F));
    }

    [[nodiscard]] inline ::testing::AssertionResult getApproxEqual(
        const Mat4 &leftVector, const Mat4 &rightVector)
    {
        for (int column = 0; column < 4; ++column)
        {
            const auto result =
                getApproxEqual(leftVector[column], rightVector[column]);

            if (!result)
            {
                return ::testing::AssertionFailure()
                    << "column " << column << ": " << result.message();
            }
        }

        return ::testing::AssertionSuccess();
    }

}
