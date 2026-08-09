#include <gtest/gtest.h>

#include <antwika/animation/KeyFrame.hpp>

namespace antwika::animation
{

    TEST(KeyFrameTest, Ctor_DefaultsToTheFirstIndexForOneTick)
    {
        const KeyFrame keyFrame;

        EXPECT_EQ(keyFrame.index, 0U);
        EXPECT_EQ(keyFrame.durationTicks, 1U);
    }

    TEST(KeyFrameTest, OperatorEquals_ComparesBothFields)
    {
        constexpr KeyFrame first{.index = 2, .durationTicks = 3};

        EXPECT_EQ(first, (KeyFrame{.index = 2, .durationTicks = 3}));
        EXPECT_NE(first, (KeyFrame{.index = 5, .durationTicks = 3}));
        EXPECT_NE(first, (KeyFrame{.index = 2, .durationTicks = 9}));
    }

}
