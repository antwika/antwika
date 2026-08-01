#include "antwika/network/NetworkError.hpp"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

using antwika::network::NetworkError;

TEST(NetworkErrorTest, CarriesTheMessageItWasGiven)
{
    const NetworkError error("no route");

    EXPECT_EQ(std::string(error.what()), "no route");
}

// One catchable type per failure category, as the rest of the tree has.
TEST(NetworkErrorTest, IsCaughtAsARuntimeError)
{
    try
    {
        throw NetworkError("refused");
    }
    catch (const std::runtime_error &error)
    {
        EXPECT_EQ(std::string(error.what()), "refused");
        return;
    }

    FAIL() << "a NetworkError was not caught as a std::runtime_error";
}
