#include <gtest/gtest.h>

#include <antwika/animation/Frame.hpp>
#include <antwika/animation/Progress.hpp>

namespace antwika::animation
{

    TEST(FrameTest, Construct_DefaultsToTheFirstFrameAtItsStart)
    {
        const Frame frame;

        EXPECT_EQ(frame.index, 0U);
        EXPECT_EQ(frame.progress, Progress(0, 1));
        EXPECT_FALSE(frame.finished);
    }

    TEST(FrameTest, Equality_ComparesEveryField)
    {
        const Frame frame{
            .index = 2,
            .progress = Progress(1, 3),
            .finished = false,
        };

        EXPECT_EQ(frame, (Frame{.index = 2,
                                .progress = Progress(1, 3),
                                .finished = false}));
        EXPECT_NE(frame, (Frame{.index = 7,
                                .progress = Progress(1, 3),
                                .finished = false}));
        EXPECT_NE(frame, (Frame{.index = 2,
                                .progress = Progress(2, 3),
                                .finished = false}));
        EXPECT_NE(frame, (Frame{.index = 2,
                                .progress = Progress(1, 3),
                                .finished = true}));
    }

} // namespace antwika::animation
