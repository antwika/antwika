#include <gtest/gtest.h>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/TextLayout.hpp>

#include "antwika/game/OverlayLabel.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/IsoProjection.hpp"

namespace
{
    using antwika::game::Camera;
    using antwika::game::Cell;
    using antwika::game::cellCentre;
    using antwika::game::overlayLabelFor;
    using antwika::gfx::Point;
    using antwika::gfx::textSize;

    constexpr Cell kCell{.x = 3, .y = 4};
}

TEST(OverlayLabelTest, OverlayLabelFor_CentresTheValueOnItsTile)
{
    const Camera camera(Point{.x = 100, .y = 20});

    const auto label = overlayLabelFor("100", kCell, camera);

    ASSERT_TRUE(label.has_value());

    const auto written = textSize("100", label->scale);
    const auto centre = cellCentre(kCell, camera);

    EXPECT_EQ(
        label->origin.x,
        centre.x - static_cast<std::int32_t>(written.width / 2));
    EXPECT_EQ(
        label->origin.y,
        centre.y - static_cast<std::int32_t>(written.height / 2));
}

TEST(OverlayLabelTest, OverlayLabelFor_WritesBiggerOnABiggerTile)
{
    const auto near = overlayLabelFor("100", kCell, Camera(Point{}, 4));
    const auto far = overlayLabelFor("100", kCell, Camera(Point{}, 3));

    ASSERT_TRUE(near.has_value());
    ASSERT_TRUE(far.has_value());

    EXPECT_EQ(near->scale, 4U);
    EXPECT_EQ(far->scale, 2U);
}

TEST(OverlayLabelTest, OverlayLabelFor_WritesNothingOnATooNarrowTile)
{
    const auto label = overlayLabelFor("100", kCell, Camera(Point{}, 1));

    EXPECT_FALSE(label.has_value());
}

TEST(OverlayLabelTest, OverlayLabelFor_WritesNothingOnATooShortTile)
{
    const auto label = overlayLabelFor("5", kCell, Camera(Point{}, 0));

    EXPECT_FALSE(label.has_value());
}

TEST(OverlayLabelTest, OverlayLabelFor_StillWritesAShortValueWhenZoomedOut)
{
    const auto label = overlayLabelFor("5", kCell, Camera(Point{}, 1));

    ASSERT_TRUE(label.has_value());
    EXPECT_EQ(label->scale, 1U);
}
