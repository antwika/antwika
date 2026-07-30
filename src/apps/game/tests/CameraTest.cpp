#include <gtest/gtest.h>

#include <cstddef>
#include <vector>

#include <antwika/gfx/Point.hpp>

#include "antwika/game/Camera.hpp"

using antwika::game::Camera;
using antwika::game::kDefaultZoomLevel;
using antwika::game::kZoomHalfWidths;
using antwika::gfx::Point;

TEST(CameraTest, DefaultConstructedSitsAtTheOriginAndDefaultZoom)
{
    constexpr Camera camera;

    EXPECT_EQ(camera.pan(), Point{});
    EXPECT_EQ(camera.zoomLevel(), kDefaultZoomLevel);
}

TEST(CameraTest, HalfHeightIsAlwaysHalfTheHalfWidth)
{
    for (std::size_t zoom = 0; zoom < kZoomHalfWidths.size(); ++zoom)
    {
        const Camera camera(Point{}, zoom);

        EXPECT_EQ(camera.halfWidth(), kZoomHalfWidths[zoom]);
        EXPECT_EQ(camera.halfHeight(), kZoomHalfWidths[zoom] / 2);
    }
}

TEST(CameraTest, HalfWidthIsNeverZero)
{
    // A zero-width tile would make the projection's divisor zero.
    for (std::size_t zoom = 0; zoom < kZoomHalfWidths.size(); ++zoom)
    {
        const Camera camera(Point{}, zoom);

        EXPECT_GT(camera.halfWidth(), 0U);
        EXPECT_GT(camera.halfHeight(), 0U);
    }
}

TEST(CameraTest, ConstructorClampsAZoomLevelPastTheEnd)
{
    // The levels come out of a vector rather than being written inline.
    // A constexpr constructor given a literal is folded at compile time.
    // An inline one would exercise no code at all.
    const std::vector<std::size_t> levels{
        kZoomHalfWidths.size(), kZoomHalfWidths.size() + 10};

    for (const auto level : levels)
    {
        const Camera camera(Point{}, level);

        EXPECT_EQ(camera.zoomLevel(), kZoomHalfWidths.size() - 1);
    }
}

TEST(CameraTest, ConstructorKeepsALevelInsideTheTable)
{
    const std::vector<std::size_t> levels{0, 1, kZoomHalfWidths.size() - 1};

    for (const auto level : levels)
    {
        const Camera camera(Point{}, level);

        EXPECT_EQ(camera.zoomLevel(), level);
    }
}

TEST(CameraTest, PanBy_AccumulatesAndLeavesTheZoomAlone)
{
    Camera camera(Point{.x = 10, .y = 20}, 2);

    camera.panBy(5, -3);
    camera.panBy(-1, 1);

    EXPECT_EQ(camera.pan(), (Point{.x = 14, .y = 18}));
    EXPECT_EQ(camera.zoomLevel(), 2U);
}

TEST(CameraTest, SetPan_ReplacesTheOffsetOutright)
{
    Camera camera(Point{.x = 10, .y = 20});

    camera.setPan(Point{.x = -4, .y = 7});

    EXPECT_EQ(camera.pan(), (Point{.x = -4, .y = 7}));
}

TEST(CameraTest, ZoomIn_StepsOneLevelCloser)
{
    Camera camera(Point{}, 1);

    camera.zoomIn();

    EXPECT_EQ(camera.zoomLevel(), 2U);
}

TEST(CameraTest, ZoomOut_StepsOneLevelFurther)
{
    Camera camera(Point{}, 1);

    camera.zoomOut();

    EXPECT_EQ(camera.zoomLevel(), 0U);
}

TEST(CameraTest, ZoomIn_ClampsAtTheClosestLevel)
{
    Camera camera(Point{}, kZoomHalfWidths.size() - 1);

    camera.zoomIn();
    camera.zoomIn();

    EXPECT_EQ(camera.zoomLevel(), kZoomHalfWidths.size() - 1);
}

TEST(CameraTest, ZoomOut_ClampsAtTheFurthestLevel)
{
    Camera camera(Point{}, 0);

    camera.zoomOut();
    camera.zoomOut();

    EXPECT_EQ(camera.zoomLevel(), 0U);
}

TEST(CameraTest, ZoomingDoesNotMoveThePan)
{
    Camera camera(Point{.x = 8, .y = 9}, 2);

    camera.zoomIn();
    camera.zoomOut();

    EXPECT_EQ(camera.pan(), (Point{.x = 8, .y = 9}));
}

TEST(CameraTest, EqualityComparesThePanAndTheZoom)
{
    const Camera camera(Point{.x = 1, .y = 2}, 2);

    EXPECT_EQ(camera, Camera(Point{.x = 1, .y = 2}, 2));
    EXPECT_NE(camera, Camera(Point{.x = 1, .y = 3}, 2));
    EXPECT_NE(camera, Camera(Point{.x = 1, .y = 2}, 3));
}
