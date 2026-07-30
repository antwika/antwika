#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>

#include "DiamondSpans.hpp"

using antwika::game::detail::fillDiamond;
using antwika::gfx::Color;
using antwika::gfx::Point;
using antwika::gfx::mocks::MockRenderer;
using ::testing::_;
using ::testing::NiceMock;

namespace
{
    constexpr Color kInk{.red = 1, .green = 2, .blue = 3};

    struct Span
    {
        Point from;
        Point to;
    };

    class Spans : public NiceMock<MockRenderer>
    {
    public:
        Spans()
        {
            ON_CALL(*this, drawLine(_, _, _))
                .WillByDefault([this](Point from, Point to, Color)
                               { rows.push_back(Span{from, to}); });
        }

        std::vector<Span> rows;
    };
} // namespace

TEST(DiamondSpansTest, FillDiamond_DrawsOneRowPerPixelOfHeight)
{
    Spans renderer;

    fillDiamond(renderer, Point{.x = 50, .y = 50}, 8, 4, kInk);

    EXPECT_EQ(renderer.rows.size(), 9U);
}

TEST(DiamondSpansTest, FillDiamond_TapersToAPointAtBothCorners)
{
    Spans renderer;

    fillDiamond(renderer, Point{.x = 50, .y = 50}, 8, 4, kInk);

    ASSERT_FALSE(renderer.rows.empty());

    // The top and bottom rows are single pixels.
    // That is why drawLine has to include both of its endpoints.
    EXPECT_EQ(renderer.rows.front().from, renderer.rows.front().to);
    EXPECT_EQ(renderer.rows.back().from, renderer.rows.back().to);
    EXPECT_EQ(renderer.rows.front().from.y, 46);
    EXPECT_EQ(renderer.rows.back().from.y, 54);
}

TEST(DiamondSpansTest, FillDiamond_IsWidestThroughItsMiddle)
{
    Spans renderer;

    fillDiamond(renderer, Point{.x = 50, .y = 50}, 8, 4, kInk);

    const auto &middle = renderer.rows[4];

    EXPECT_EQ(middle.from, (Point{.x = 42, .y = 50}));
    EXPECT_EQ(middle.to, (Point{.x = 58, .y = 50}));
}

TEST(DiamondSpansTest, FillDiamond_IsSymmetricAboutItsCentreRow)
{
    Spans renderer;

    fillDiamond(renderer, Point{.x = 50, .y = 50}, 8, 4, kInk);

    const auto rows = renderer.rows.size();
    for (std::size_t i = 0; i < rows; ++i)
    {
        const auto &above = renderer.rows[i];
        const auto &below = renderer.rows[rows - 1 - i];

        EXPECT_EQ(above.to.x - above.from.x, below.to.x - below.from.x)
            << "row " << i;
    }
}

TEST(DiamondSpansTest, FillDiamond_CentresEveryRowOnTheGivenPoint)
{
    Spans renderer;

    fillDiamond(renderer, Point{.x = 50, .y = 50}, 8, 4, kInk);

    for (const auto &row : renderer.rows)
    {
        EXPECT_EQ(50 - row.from.x, row.to.x - 50) << "row " << row.from.y;
    }
}

TEST(DiamondSpansTest, FillDiamond_DrawsOneLineForAFlatDiamond)
{
    Spans renderer;

    // Dividing by the height to taper the rows would divide by zero.
    fillDiamond(renderer, Point{.x = 10, .y = 20}, 6, 0, kInk);

    ASSERT_EQ(renderer.rows.size(), 1U);
    EXPECT_EQ(renderer.rows[0].from, (Point{.x = 4, .y = 20}));
    EXPECT_EQ(renderer.rows[0].to, (Point{.x = 16, .y = 20}));
}

TEST(DiamondSpansTest, FillDiamond_TreatsANegativeHeightAsFlat)
{
    Spans renderer;

    fillDiamond(renderer, Point{.x = 10, .y = 20}, 6, -3, kInk);

    EXPECT_EQ(renderer.rows.size(), 1U);
}

TEST(DiamondSpansTest, FillDiamond_DrawsAtNegativeCoordinates)
{
    Spans renderer;

    fillDiamond(renderer, Point{.x = -100, .y = -100}, 4, 2, kInk);

    ASSERT_EQ(renderer.rows.size(), 5U);
    EXPECT_EQ(renderer.rows[2].from, (Point{.x = -104, .y = -100}));
    EXPECT_EQ(renderer.rows[2].to, (Point{.x = -96, .y = -100}));
}

TEST(DiamondSpansTest, FillDiamond_DrawsASinglePixelForNoSizeAtAll)
{
    Spans renderer;

    fillDiamond(renderer, Point{.x = 7, .y = 8}, 0, 0, kInk);

    ASSERT_EQ(renderer.rows.size(), 1U);
    EXPECT_EQ(renderer.rows[0].from, renderer.rows[0].to);
}
