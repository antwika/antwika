#include "antwika/pattern/PatternError.hpp"

#include <stdexcept>
#include <string>

#include <gtest/gtest.h>

using antwika::pattern::PatternError;

TEST(PatternErrorTest, CarriesTheMessageItWasGiven)
{
    const PatternError error("a span holds no time");

    EXPECT_EQ(std::string(error.what()), "a span holds no time");
}

// Refusing rather than rounding is the whole point.
// A rounded position is a note somewhere near where the score said it.
TEST(PatternErrorTest, IsARuntimeError)
{
    EXPECT_THROW(throw PatternError("nope"), std::runtime_error);
}
