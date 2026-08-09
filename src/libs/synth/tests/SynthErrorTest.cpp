#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

#include "antwika/synth/SynthError.hpp"

using antwika::synth::SynthError;

TEST(SynthErrorTest, What_CarriesTheMessageItWasGiven)
{
    const SynthError error("a voice would never be heard");

    EXPECT_EQ(std::string(error.what()), "a voice would never be heard");
}

TEST(SynthErrorTest, Ctor_IsCaughtAsARuntimeError)
{
    EXPECT_THROW(throw SynthError("nope"), std::runtime_error);
}
