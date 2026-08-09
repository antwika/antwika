#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

#include "antwika/font/FontError.hpp"

using antwika::font::FontError;

TEST(FontErrorTest, What_CarriesTheMessageItWasGiven)
{
    const FontError error("these bytes are not a font");

    EXPECT_EQ(std::string(error.what()), "these bytes are not a font");
}

TEST(FontErrorTest, Ctor_IsCaughtAsARuntimeError)
{
    EXPECT_THROW(throw FontError("nope"), std::runtime_error);
}
