#include <gtest/gtest.h>

#include <antwika/gfx/mocks/MockRenderer.hpp>

using antwika::gfx::mocks::MockRenderer;

TEST(IRendererTest, Renderer3d_IsAbsentUnlessARendererOffersOne)
{
    MockRenderer renderer;

    EXPECT_EQ(nullptr, renderer.renderer3d());
}
