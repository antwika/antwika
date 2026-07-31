#include "antwika/ttf/TtfError.hpp"

#include <stdexcept>
#include <string>

#include <gtest/gtest.h>

using antwika::ttf::TtfError;

TEST(TtfErrorTest, CarriesTheMessageItWasGiven)
{
    const TtfError error("these bytes are not a font");

    EXPECT_EQ(std::string(error.what()), "these bytes are not a font");
}

// One catchable type per failure category.
// A caller never learns which rasteriser this was built against.
TEST(TtfErrorTest, IsARuntimeError)
{
    EXPECT_THROW(throw TtfError("nope"), std::runtime_error);
}
