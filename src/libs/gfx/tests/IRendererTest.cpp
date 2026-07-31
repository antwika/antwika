#include <gtest/gtest.h>

#include <antwika/gfx/mocks/MockRenderer.hpp>

using antwika::gfx::mocks::MockRenderer;

TEST(IRendererTest, Renderer3d_IsAbsentUnlessARendererOffersOne)
{
    MockRenderer renderer;

    // The default is what keeps the 2D interface additive.
    // A renderer with no 3D path says so, rather than dropping a draw.
    EXPECT_EQ(nullptr, renderer.renderer3d());
}
