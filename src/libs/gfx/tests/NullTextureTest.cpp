#include <gtest/gtest.h>

#include "antwika/gfx/Size.hpp"

#include "NullTexture.hpp"

using antwika::gfx::Size;
using antwika::gfx::detail::NullTexture;

TEST(NullTextureTest, Size_ReportsTheSizeItWasCreatedWith)
{
    const NullTexture texture(Size{.width = 16, .height = 9});

    EXPECT_EQ(texture.getSize(), (Size{.width = 16, .height = 9}));
}
