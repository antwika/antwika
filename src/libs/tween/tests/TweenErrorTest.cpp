#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

#include <antwika/tween/TweenError.hpp>

namespace antwika::tween
{

    TEST(TweenErrorTest, What_KeepsTheMessageItWasGiven)
    {
        const TweenError error("does not fit");

        EXPECT_EQ(std::string(error.what()), "does not fit");
    }

    TEST(TweenErrorTest, Catch_IsARuntimeError)
    {
        EXPECT_THROW(throw TweenError("does not fit"), std::runtime_error);
    }

} // namespace antwika::tween
