#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

#include "antwika/sequencer/SequencerError.hpp"

using antwika::sequencer::SequencerError;

TEST(SequencerErrorTest, What_CarriesTheMessageItWasGiven)
{
    const SequencerError error("a tick lasts no time");

    EXPECT_EQ(std::string(error.what()), "a tick lasts no time");
}

TEST(SequencerErrorTest, Ctor_IsCaughtAsARuntimeError)
{
    EXPECT_THROW(throw SequencerError("nope"), std::runtime_error);
}
