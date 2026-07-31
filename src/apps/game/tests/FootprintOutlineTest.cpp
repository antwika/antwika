#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>

#include <antwika/gfx/Point.hpp>

#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/FootprintOutline.hpp"
#include "antwika/game/IsoProjection.hpp"

using antwika::game::Camera;
using antwika::game::Cell;
using antwika::game::cellToScreen;
using antwika::game::Footprint;
using antwika::game::footprintBounds;
using antwika::game::footprintOutline;
using antwika::game::kOutlineCorners;
using antwika::gfx::Point;

namespace
{
    constexpr Cell kOrigin{.x = 3, .y = 2};

    // The corners, in the order footprintOutline() reports them.
    constexpr std::size_t kNorth = 0;
    constexpr std::size_t kEast = 1;
    constexpr std::size_t kSouth = 2;
    constexpr std::size_t kWest = 3;
} // namespace

TEST(FootprintOutlineTest, FootprintOutline_StartsAtTheBlocksTopCorner)
{
    const Camera camera;

    const auto outline =
        footprintOutline(kOrigin, Footprint{2, 2}, camera);

    // Which is the origin cell's own top corner, the block being square.
    EXPECT_EQ(outline[kNorth], cellToScreen(kOrigin, camera));
}

// Traced round the very box the block's tile is blitted into.
// Working the two out separately is what this rules out.
TEST(FootprintOutlineTest, FootprintOutline_SitsOnTheBlocksOwnBox)
{
    const Camera camera;
    constexpr Footprint footprint{3, 3};

    const auto box = footprintBounds(kOrigin, footprint, camera);
    const auto outline = footprintOutline(kOrigin, footprint, camera);

    const auto right =
        box.origin.x + static_cast<std::int32_t>(box.size.width) - 1;
    const auto bottom =
        box.origin.y + static_cast<std::int32_t>(box.size.height) - 1;

    EXPECT_EQ(outline[kNorth].y, box.origin.y);
    EXPECT_EQ(outline[kSouth].y, bottom);
    EXPECT_EQ(outline[kWest].x, box.origin.x);
    EXPECT_EQ(outline[kEast].x, right);

    // A diamond, so the two pairs share an axis.
    EXPECT_EQ(outline[kNorth].x, outline[kSouth].x);
    EXPECT_EQ(outline[kEast].y, outline[kWest].y);
}

// The projection is 2:1, so every edge of the diamond is.
// A border that was not would not lie along the tile beneath it.
TEST(FootprintOutlineTest, FootprintOutline_KeepsTheTwoToOneSlope)
{
    const Camera camera;

    const auto outline =
        footprintOutline(kOrigin, Footprint{2, 2}, camera);

    const auto down = outline[kWest].y - outline[kNorth].y;

    EXPECT_EQ(outline[kNorth].x - outline[kWest].x, 2 * down);

    // One pixel short of it eastward, and deliberately so.
    // The box's last column is a pixel inside its width.
    // So the border lands on the block rather than past it.
    EXPECT_EQ(outline[kEast].x - outline[kNorth].x, 2 * down - 1);
}

// One cell is a footprint of one, so the two answers have to agree.
TEST(FootprintOutlineTest, FootprintOutline_BoundsOneCellForAFootprintOfOne)
{
    const Camera camera;

    const auto outline =
        footprintOutline(kOrigin, Footprint{1, 1}, camera);
    const auto box = footprintBounds(kOrigin, Footprint{1, 1}, camera);

    EXPECT_EQ(outline[kNorth], cellToScreen(kOrigin, camera));
    EXPECT_EQ(
        outline[kEast].x,
        box.origin.x + static_cast<std::int32_t>(box.size.width) - 1);
}

// A bigger block is a bigger diamond hung from the same corner.
TEST(FootprintOutlineTest, FootprintOutline_GrowsWithTheFootprint)
{
    const Camera camera;

    const auto small = footprintOutline(kOrigin, Footprint{1, 1}, camera);
    const auto large = footprintOutline(kOrigin, Footprint{3, 3}, camera);

    EXPECT_EQ(small[kNorth], large[kNorth]);
    EXPECT_GT(large[kSouth].y, small[kSouth].y);
    EXPECT_GT(large[kEast].x, small[kEast].x);
    EXPECT_LT(large[kWest].x, small[kWest].x);
}

// Which pixel a cell is at is the camera's answer, and only its.
TEST(FootprintOutlineTest, FootprintOutline_MovesWithTheCamera)
{
    constexpr Footprint footprint{2, 2};
    const Camera home;
    const Camera panned(Point{.x = 40, .y = 25});

    const auto here = footprintOutline(kOrigin, footprint, home);
    const auto there = footprintOutline(kOrigin, footprint, panned);

    for (std::size_t corner = 0; corner < kOutlineCorners; ++corner)
    {
        EXPECT_EQ(there[corner].x, here[corner].x + 40);
        EXPECT_EQ(there[corner].y, here[corner].y + 25);
    }
}

TEST(FootprintOutlineTest, FootprintOutline_ZoomsWithTheCamera)
{
    constexpr Footprint footprint{2, 2};
    const Camera closest(Point{}, 4);
    const Camera furthest(Point{}, 0);

    const auto wide = footprintOutline(kOrigin, footprint, closest);
    const auto narrow = footprintOutline(kOrigin, footprint, furthest);

    EXPECT_GT(
        wide[kEast].x - wide[kWest].x,
        narrow[kEast].x - narrow[kWest].x);
}
