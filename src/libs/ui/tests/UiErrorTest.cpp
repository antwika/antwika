#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

#include "antwika/ui/UiError.hpp"

using antwika::ui::UiError;

TEST(UiErrorTest, What_CarriesTheMessageItWasGiven)
{
    const UiError error("a container is still open");

    EXPECT_EQ(std::string(error.what()), "a container is still open");
}

TEST(UiErrorTest, Ctor_IsCaughtAsARuntimeError)
{
    EXPECT_THROW(throw UiError("nope"), std::runtime_error);
}
