#include "antwika/sound/SoundError.hpp"

#include <stdexcept>
#include <string>

#include <gtest/gtest.h>

using antwika::sound::SoundError;

TEST(SoundErrorTest, CarriesTheMessageItWasGiven)
{
    const SoundError error("a device would not open");

    EXPECT_EQ(std::string(error.what()), "a device would not open");
}

// One catchable type per failure category.
// A caller never learns from it which framework this was built against.
TEST(SoundErrorTest, IsARuntimeError)
{
    EXPECT_THROW(throw SoundError("nope"), std::runtime_error);
}
