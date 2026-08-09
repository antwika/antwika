#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "antwika/console/HistoryRows.hpp"

using antwika::console::historyRows;
using antwika::console::wrappedRows;

namespace
{
    using Rows = std::vector<std::string>;
}

TEST(HistoryRowsTest, WrappedRows_KeepsALineThatFitsWhole)
{
    EXPECT_EQ(wrappedRows("hello", 8), Rows{"hello"});
}

TEST(HistoryRowsTest, WrappedRows_GivesAnEmptyLineOneEmptyRow)
{
    EXPECT_EQ(wrappedRows("", 8), Rows{""});
}

TEST(HistoryRowsTest, WrappedRows_BreaksAtTheLastSpaceThatFits)
{
    const Rows expected{"the quick", "brown fox"};

    EXPECT_EQ(wrappedRows("the quick brown fox", 10), expected);
}

TEST(HistoryRowsTest, WrappedRows_EatsTheSpaceItBrokeAt)
{
    const Rows expected{"abc", "def"};

    EXPECT_EQ(wrappedRows("abc def", 3), expected);
}

TEST(HistoryRowsTest, WrappedRows_CutsAWordWiderThanTheBudget)
{
    const Rows expected{"hello", "world"};

    EXPECT_EQ(wrappedRows("helloworld", 5), expected);
}

TEST(HistoryRowsTest, WrappedRows_CutsWhenTheOnlySpaceOpensTheLine)
{
    const Rows expected{" ab", "cde", "f"};

    EXPECT_EQ(wrappedRows(" abcdef", 3), expected);
}

TEST(HistoryRowsTest, WrappedRows_ReadsAZeroBudgetAsOneColumn)
{
    const Rows expected{"a", "b"};

    EXPECT_EQ(wrappedRows("ab", 0), expected);
}

TEST(HistoryRowsTest, HistoryRows_WrapsEveryLineInOrder)
{
    const Rows lines{"one two", "three"};
    const Rows expected{"one", "two", "three"};

    EXPECT_EQ(historyRows(lines, 5, 8), expected);
}

TEST(HistoryRowsTest, HistoryRows_DropsTheOldestRowsPastTheLimit)
{
    const Rows lines{"aaa bbb ccc", "ddd"};
    const Rows expected{"ccc", "ddd"};

    EXPECT_EQ(historyRows(lines, 3, 2), expected);
}

TEST(HistoryRowsTest, HistoryRows_DropsNothingWhenTheRowsFitTheLimit)
{
    const Rows lines{"aaa", "bbb"};
    const Rows expected{"aaa", "bbb"};

    EXPECT_EQ(historyRows(lines, 3, 2), expected);
}
