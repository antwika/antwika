#include "antwika/font/FontError.hpp"

#include <stdexcept>
#include <string>

#include <gtest/gtest.h>

using antwika::font::FontError;

TEST(FontErrorTest, CarriesTheMessageItWasGiven)
{
    const FontError error("these bytes are not a font");

    EXPECT_EQ(std::string(error.what()), "these bytes are not a font");
}

// One catchable type per failure category.
// A caller never learns which rasteriser this was built against.
TEST(FontErrorTest, IsARuntimeError)
{
    EXPECT_THROW(throw FontError("nope"), std::runtime_error);
}
