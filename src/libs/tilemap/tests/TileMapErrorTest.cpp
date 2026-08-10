#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

#include <antwika/tilemap/TileMapError.hpp>

using antwika::tilemap::TileMapError;

TEST(TileMapErrorTest, What_CarriesTheMessageItWasGiven)
{
    const TileMapError error("the cell lies outside the map");

    EXPECT_EQ(std::string(error.what()), "the cell lies outside the map");
}

TEST(TileMapErrorTest, Ctor_IsCaughtAsARuntimeError)
{
    EXPECT_THROW(throw TileMapError("nope"), std::runtime_error);
}
