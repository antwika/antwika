#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

#include <antwika/tileset/TilesetError.hpp>

using antwika::tileset::TilesetError;

TEST(TilesetErrorTest, What_CarriesTheMessageItWasGiven)
{
    const TilesetError error("the tileset holds no layers");

    EXPECT_EQ(
        std::string(error.what()), "the tileset holds no layers");
}

TEST(TilesetErrorTest, Ctor_IsCaughtAsARuntimeError)
{
    EXPECT_THROW(
        throw TilesetError("nope"), std::runtime_error);
}
