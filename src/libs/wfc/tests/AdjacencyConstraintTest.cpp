#include <gtest/gtest.h>

#include <vector>

#include <antwika/wfc/AdjacencyConstraint.hpp>
#include <antwika/wfc/CompatibilityTable.hpp>
#include <antwika/wfc/Domain.hpp>

using antwika::wfc::AdjacencyConstraint;
using antwika::wfc::CompatibilityTable;
using antwika::wfc::Domain;

namespace
{
    CompatibilityTable makeTable()
    {
        CompatibilityTable table(3);
        table.set(0, 2, false);
        table.set(2, 0, false);
        return table;
    }

    CompatibilityTable makeTablePruningAValueFromEachSide()
    {
        CompatibilityTable table(3);
        table.set(2, 0, false);
        table.set(2, 1, false);
        table.set(2, 2, false);
        table.set(0, 0, false);
        table.set(1, 0, false);
        return table;
    }
}

TEST(AdjacencyConstraintTest, Cells_ReturnsLeftAndRight)
{
    AdjacencyConstraint constraint(3, 7, makeTable());
    std::vector<std::size_t> cells(
        constraint.cells().begin(), constraint.cells().end());
    EXPECT_EQ(cells, (std::vector<std::size_t>{3, 7}));
}

TEST(AdjacencyConstraintTest, Prune_RemovesIncompatibleValuesBothSides)
{
    AdjacencyConstraint constraint(0, 1, makeTablePruningAValueFromEachSide());
    std::vector<Domain> wave{Domain(3), Domain(3)};

    EXPECT_TRUE(constraint.prune(wave));
    EXPECT_TRUE(wave[0].contains(0));
    EXPECT_TRUE(wave[0].contains(1));
    EXPECT_FALSE(wave[0].contains(2));
    EXPECT_FALSE(wave[1].contains(0));
    EXPECT_TRUE(wave[1].contains(1));
    EXPECT_TRUE(wave[1].contains(2));
}

TEST(AdjacencyConstraintTest, Prune_FailsWithNoCompatiblePartner)
{
    AdjacencyConstraint constraint(0, 1, makeTable());
    std::vector<Domain> wave{
        Domain::singleton(0, 3), Domain::singleton(2, 3)};

    EXPECT_FALSE(constraint.prune(wave));
}

TEST(
    AdjacencyConstraintTest,
    Prune_IsSafeWhenTheTableIsSmaller)
{
    CompatibilityTable table(2);
    AdjacencyConstraint constraint(0, 1, table);
    std::vector<Domain> wave{Domain(4), Domain(4)};

    EXPECT_TRUE(constraint.prune(wave));
    EXPECT_FALSE(wave[0].contains(2));
    EXPECT_FALSE(wave[0].contains(3));
    EXPECT_TRUE(wave[0].contains(0));
    EXPECT_TRUE(wave[0].contains(1));
    EXPECT_FALSE(wave[1].contains(2));
    EXPECT_FALSE(wave[1].contains(3));
    EXPECT_TRUE(wave[1].contains(0));
    EXPECT_TRUE(wave[1].contains(1));
}
