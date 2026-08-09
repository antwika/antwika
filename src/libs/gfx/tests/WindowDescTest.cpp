#include <gtest/gtest.h>

#include "antwika/gfx/Size.hpp"
#include "antwika/gfx/WindowDesc.hpp"

using antwika::gfx::Size;
using antwika::gfx::WindowDesc;

TEST(WindowDescTest, Defaults_AskForAnUntitledFixed800x600Window)
{
    const WindowDesc desc;

    EXPECT_TRUE(desc.title.empty());
    EXPECT_EQ(desc.size, (Size{.width = 800, .height = 600}));
    EXPECT_FALSE(desc.resizable);
    EXPECT_FALSE(desc.fullscreen);
}
