#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

#include <antwika/animation/AnimationError.hpp>

namespace antwika::animation
{

    TEST(AnimationErrorTest, What_KeepsTheMessageItWasGiven)
    {
        const AnimationError error("bad clip");

        EXPECT_EQ(std::string(error.what()), "bad clip");
    }

    TEST(AnimationErrorTest, Catch_IsARuntimeError)
    {
        EXPECT_THROW(throw AnimationError("bad clip"), std::runtime_error);
    }

} // namespace antwika::animation
