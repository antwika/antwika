#include <gtest/gtest.h>

#include <antwika/wfc/CompatibilityTable.hpp>

using antwika::wfc::CompatibilityTable;

TEST(CompatibilityTableTest, Compatible_DefaultsToEveryPair)
{
    CompatibilityTable table(3);
    for (std::size_t left = 0; left < 3; ++left)
    {
        for (std::size_t right = 0; right < 3; ++right)
        {
            EXPECT_TRUE(table.compatible(left, right));
        }
    }
}

TEST(CompatibilityTableTest, Set_MarksExactlyThatOrderedPair)
{
    CompatibilityTable table(3);
    table.set(0, 2, false);

    EXPECT_FALSE(table.compatible(0, 2));
    EXPECT_TRUE(table.compatible(2, 0));
}

TEST(CompatibilityTableTest, AlphabetSize_IsPreserved)
{
    CompatibilityTable table(5);
    EXPECT_EQ(table.alphabetSize(), 5U);
}

TEST(CompatibilityTableTest, Compatible_IsFalseOnAnOutOfRangePair)
{
    CompatibilityTable table(2);

    EXPECT_FALSE(table.compatible(2, 0));
    EXPECT_FALSE(table.compatible(0, 2));
    EXPECT_FALSE(table.compatible(2, 2));
}

TEST(CompatibilityTableTest, Set_IgnoresAnOutOfRangePair)
{
    CompatibilityTable table(2);

    table.set(2, 0, false);
    table.set(0, 2, false);

    EXPECT_TRUE(table.compatible(0, 0));
    EXPECT_TRUE(table.compatible(0, 1));
    EXPECT_TRUE(table.compatible(1, 0));
    EXPECT_TRUE(table.compatible(1, 1));
}
