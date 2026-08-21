#include <gtest/gtest.h>

#include "antwika/gfx/ShaderSource.hpp"

using antwika::gfx::ShaderSource;

TEST(ShaderSourceTest, IsComplete_WantsBothStages)
{
    EXPECT_TRUE((ShaderSource{.vertex = "vs", .fragment = "fs"})
                    .isComplete());
}

TEST(ShaderSourceTest, IsComplete_RejectsAnEmptySource)
{
    EXPECT_FALSE(ShaderSource{}.isComplete());
}

TEST(ShaderSourceTest, IsComplete_RejectsAMissingFragmentStage)
{
    EXPECT_FALSE(
        (ShaderSource{.vertex = "vs", .fragment = ""}).isComplete());
}

TEST(ShaderSourceTest, IsComplete_RejectsAMissingVertexStage)
{
    EXPECT_FALSE(
        (ShaderSource{.vertex = "", .fragment = "fs"}).isComplete());
}

TEST(ShaderSourceTest, OperatorEquals_ComparesBothStages)
{
    const ShaderSource leftSource{.vertex = "vs", .fragment = "fs"};

    EXPECT_EQ(leftSource, (ShaderSource{.vertex = "vs", .fragment = "fs"}));
    EXPECT_NE(leftSource, (ShaderSource{.vertex = "other", .fragment = "fs"}));
    EXPECT_NE(leftSource, (ShaderSource{.vertex = "vs", .fragment = "other"}));
}
