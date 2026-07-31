#pragma once

#include <gtest/gtest.h>

#include <cmath>

#include "antwika/gfx/Math3D.hpp"

namespace antwika::gfx::tests
{

    /// How far two floats may differ and still count as equal.
    constexpr float kEpsilon = 1e-5F;

    /**
     * @brief Compare two vectors component by component.
     * @param left One vector.
     * @param right The other.
     * @return A gtest result naming the first component that differs
     * by more than kEpsilon, so a failure says which one.
     */
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

    /**
     * @brief Compare two vectors component by component.
     * @param left One vector.
     * @param right The other.
     * @return A gtest result naming the first component that differs
     * by more than kEpsilon.
     */
    [[nodiscard]] inline ::testing::AssertionResult approxEqual(
        const Vec3 &left, const Vec3 &right)
    {
        return approxEqual(Vec4(left, 0.0F), Vec4(right, 0.0F));
    }

    /**
     * @brief Compare two matrices column by column.
     * @param left One matrix.
     * @param right The other.
     * @return A gtest result naming the first column that differs by
     * more than kEpsilon.
     */
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

} // namespace antwika::gfx::tests
