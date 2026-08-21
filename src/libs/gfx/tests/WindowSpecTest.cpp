#include <gtest/gtest.h>

#include "antwika/gfx/Size.hpp"
#include "antwika/gfx/WindowSpec.hpp"

using antwika::gfx::Size;
using antwika::gfx::WindowSpec;

TEST(WindowSpecTest, Defaults_AskForAnUntitledFixed800x600Window)
{
    const WindowSpec spec;

    EXPECT_TRUE(spec.title.empty());
    EXPECT_EQ(spec.size, (Size{.width = 800, .height = 600}));
    EXPECT_FALSE(spec.resizable);
    EXPECT_FALSE(spec.fullscreen);
}
