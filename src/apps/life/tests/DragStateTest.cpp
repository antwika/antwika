#include <gtest/gtest.h>

#include "antwika/life/DragState.hpp"

using antwika::life::DragState;

TEST(DragStateTest, InProgress_IsFalseBeforeAnythingHappens)
{
    const DragState drag;

    EXPECT_FALSE(drag.inProgress());
}

TEST(DragStateTest, Begin_StartsADrag)
{
    DragState drag;

    drag.begin();

    EXPECT_TRUE(drag.inProgress());
}

TEST(DragStateTest, End_FinishesADrag)
{
    DragState drag;
    drag.begin();

    drag.end();

    EXPECT_FALSE(drag.inProgress());
}

TEST(DragStateTest, Begin_IsIdempotent)
{
    DragState drag;

    drag.begin();
    drag.begin();

    EXPECT_TRUE(drag.inProgress());
}

TEST(DragStateTest, End_IsIdempotent)
{
    DragState drag;

    drag.end();
    drag.end();

    EXPECT_FALSE(drag.inProgress());
}

TEST(DragStateTest, BeginAfterEnd_StartsAFreshDrag)
{
    DragState drag;
    drag.begin();
    drag.end();

    drag.begin();

    EXPECT_TRUE(drag.inProgress());
}
