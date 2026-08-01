#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

#include "antwika/ui/UiError.hpp"

using antwika::ui::UiError;

TEST(UiErrorTest, CarriesTheMessageItWasGiven)
{
    const UiError error("a container is still open");

    EXPECT_EQ(std::string(error.what()), "a container is still open");
}

// One catchable type per failure category, as every library here has.
// So a caller catches this rather than std::runtime_error.
TEST(UiErrorTest, IsARuntimeError)
{
    EXPECT_THROW(throw UiError("nope"), std::runtime_error);
}
