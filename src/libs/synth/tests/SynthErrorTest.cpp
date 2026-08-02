#include "antwika/synth/SynthError.hpp"

#include <stdexcept>
#include <string>

#include <gtest/gtest.h>

using antwika::synth::SynthError;

TEST(SynthErrorTest, CarriesTheMessageItWasGiven)
{
    const SynthError error("a voice would never be heard");

    EXPECT_EQ(std::string(error.what()), "a voice would never be heard");
}

// One catchable type per failure category.
// Raised from the trigger path and never from the render path.
TEST(SynthErrorTest, IsARuntimeError)
{
    EXPECT_THROW(throw SynthError("nope"), std::runtime_error);
}
