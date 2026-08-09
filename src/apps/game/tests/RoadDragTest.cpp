#include <gtest/gtest.h>

#include "antwika/game/Cell.hpp"
#include "antwika/game/RoadDrag.hpp"

using antwika::game::Cell;
using antwika::game::RoadDrag;

TEST(RoadDragTest, Active_IsFalseUntilOneBegins)
{
    const RoadDrag drag;

    EXPECT_FALSE(drag.active());
}

TEST(RoadDragTest, Begin_MarksTheStartAndTheEndAsOneCell)
{
    RoadDrag drag;

    drag.begin(Cell{.x = 2, .y = 3});

    EXPECT_TRUE(drag.active());
    EXPECT_EQ(drag.start(), (Cell{.x = 2, .y = 3}));
    EXPECT_EQ(drag.end(), (Cell{.x = 2, .y = 3}));
}

TEST(RoadDragTest, DragTo_MovesTheEndAndLeavesTheStart)
{
    RoadDrag drag;
    drag.begin(Cell{.x = 2, .y = 3});

    drag.dragTo(Cell{.x = 7, .y = 1});

    EXPECT_EQ(drag.start(), (Cell{.x = 2, .y = 3}));
    EXPECT_EQ(drag.end(), (Cell{.x = 7, .y = 1}));
}

TEST(RoadDragTest, DragTo_DoesNothingWhileNoDragIsUnderWay)
{
    RoadDrag drag;

    drag.dragTo(Cell{.x = 7, .y = 1});

    EXPECT_FALSE(drag.active());
    EXPECT_EQ(drag.end(), Cell{});
}

TEST(RoadDragTest, Finish_EndsTheDrag)
{
    RoadDrag drag;
    drag.begin(Cell{.x = 2, .y = 3});

    drag.finish();

    EXPECT_FALSE(drag.active());
}
