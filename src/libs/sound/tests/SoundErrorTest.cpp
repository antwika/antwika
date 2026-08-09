#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

#include "antwika/sound/SoundError.hpp"

using antwika::sound::SoundError;

TEST(SoundErrorTest, What_CarriesTheMessageItWasGiven)
{
    const SoundError error("a device would not open");

    EXPECT_EQ(std::string(error.what()), "a device would not open");
}

TEST(SoundErrorTest, Ctor_IsCaughtAsARuntimeError)
{
    EXPECT_THROW(throw SoundError("nope"), std::runtime_error);
}
