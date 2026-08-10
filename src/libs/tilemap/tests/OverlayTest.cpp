#include <gtest/gtest.h>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/tilemap/Overlay.hpp>

using antwika::tilemap::Overlay;
using antwika::tilemap::toString;

TEST(OverlayTest, Overlay_CountsTwoKinds)
{
    EXPECT_EQ(antwika::enums::kCount<Overlay>, 2U);
}

TEST(OverlayTest, ToString_NamesEveryOverlay)
{
    EXPECT_EQ(toString(Overlay::None), "none");
    EXPECT_EQ(toString(Overlay::Bridge), "bridge");
}

TEST(OverlayTest, ToString_FallsBackForAValueThatNamesNoOverlay)
{
    EXPECT_EQ(toString(static_cast<Overlay>(42)), "unknown");
}
