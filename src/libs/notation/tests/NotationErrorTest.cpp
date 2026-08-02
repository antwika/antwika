#include "antwika/notation/NotationError.hpp"

#include <stdexcept>
#include <string>

#include <gtest/gtest.h>

using antwika::notation::NotationError;

TEST(NotationErrorTest, CarriesTheMessageItWasGiven)
{
    const NotationError error("a bracket nothing closes");

    EXPECT_EQ(std::string(error.what()), "a bracket nothing closes");
}

// This library owns the grammar.
// The algebra owns what the grammar means, and refuses in its own type.
TEST(NotationErrorTest, IsARuntimeError)
{
    EXPECT_THROW(throw NotationError("nope"), std::runtime_error);
}
