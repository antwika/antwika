#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

#include "antwika/network/NetworkError.hpp"

using antwika::network::NetworkError;

TEST(NetworkErrorTest, What_CarriesTheMessageItWasGiven)
{
    const NetworkError error("no route");

    EXPECT_EQ(std::string(error.what()), "no route");
}

TEST(NetworkErrorTest, Ctor_IsCaughtAsARuntimeError)
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
