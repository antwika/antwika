#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

#include "antwika/atlas/AtlasError.hpp"

using antwika::atlas::AtlasError;

TEST(AtlasErrorTest, What_CarriesTheMessageItWasGiven)
{
    const AtlasError error("the atlas names no slots");

    EXPECT_EQ(std::string(error.what()), "the atlas names no slots");
}

TEST(AtlasErrorTest, Ctor_IsCaughtAsARuntimeError)
{
    EXPECT_THROW(throw AtlasError("nope"), std::runtime_error);
}
