#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

#include "antwika/pattern/PatternError.hpp"

using antwika::pattern::PatternError;

TEST(PatternErrorTest, What_CarriesTheMessageItWasGiven)
{
    const PatternError error("a span holds no time");

    EXPECT_EQ(std::string(error.what()), "a span holds no time");
}

TEST(PatternErrorTest, Ctor_IsCaughtAsARuntimeError)
{
    EXPECT_THROW(throw PatternError("nope"), std::runtime_error);
}
