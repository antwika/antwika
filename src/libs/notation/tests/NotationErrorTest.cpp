#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

#include "antwika/notation/NotationError.hpp"

using antwika::notation::NotationError;

TEST(NotationErrorTest, What_CarriesTheMessageItWasGiven)
{
    const NotationError error("a bracket nothing closes");

    EXPECT_EQ(std::string(error.what()), "a bracket nothing closes");
}

TEST(NotationErrorTest, Ctor_IsCaughtAsARuntimeError)
{
    EXPECT_THROW(throw NotationError("nope"), std::runtime_error);
}
