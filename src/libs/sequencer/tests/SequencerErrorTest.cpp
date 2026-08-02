#include "antwika/sequencer/SequencerError.hpp"

#include <stdexcept>
#include <string>

#include <gtest/gtest.h>

using antwika::sequencer::SequencerError;

TEST(SequencerErrorTest, CarriesTheMessageItWasGiven)
{
    const SequencerError error("a tick lasts no time");

    EXPECT_EQ(std::string(error.what()), "a tick lasts no time");
}

// Arithmetic that will not fit is antwika::pattern's refusal instead.
// That is where the exact rational lives.
TEST(SequencerErrorTest, IsARuntimeError)
{
    EXPECT_THROW(throw SequencerError("nope"), std::runtime_error);
}
