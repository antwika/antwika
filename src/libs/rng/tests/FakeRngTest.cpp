#include <gtest/gtest.h>

#include <cstdint>

#include "antwika/rng/fakes/FakeRng.hpp"

using antwika::rng::fakes::FakeRng;

TEST(FakeRngTest, Next_HandsBackTheScriptedValuesInOrder)
{
    FakeRng rng({7, 8, 9});
    EXPECT_EQ(rng.next(), 7U);
    EXPECT_EQ(rng.next(), 8U);
    EXPECT_EQ(rng.next(), 9U);
}

TEST(FakeRngTest, Next_WrapsToTheStartOnceExhausted)
{
    FakeRng rng({1, 2});
    EXPECT_EQ(rng.next(), 1U);
    EXPECT_EQ(rng.next(), 2U);
    EXPECT_EQ(rng.next(), 1U);
    EXPECT_EQ(rng.next(), 2U);
}
