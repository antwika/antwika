#include <antwika/wfc/AdjacencyConstraint.hpp>

#include <vector>

#include <gtest/gtest.h>

#include <antwika/wfc/CompatibilityTable.hpp>
#include <antwika/wfc/Domain.hpp>

using antwika::wfc::AdjacencyConstraint;
using antwika::wfc::CompatibilityTable;
using antwika::wfc::Domain;

namespace
{
    // Symbols: 0 = grass, 1 = sand, 2 = water.
    // grass never sits directly next to water.
    CompatibilityTable makeTable()
    {
        CompatibilityTable table(3);
        table.set(0, 2, false);
        table.set(2, 0, false);
        return table;
    }
} // namespace

TEST(AdjacencyConstraintTest, CompatibilityTableReportsAlphabetSize)
{
    CompatibilityTable table = makeTable();
    EXPECT_EQ(table.alphabetSize(), 3U);
}

TEST(AdjacencyConstraintTest, CellsReturnsLeftAndRight)
{
    AdjacencyConstraint constraint(3, 7, makeTable());
    std::vector<std::size_t> cells(
        constraint.cells().begin(), constraint.cells().end());
    EXPECT_EQ(cells, (std::vector<std::size_t>{3, 7}));
}

TEST(AdjacencyConstraintTest, IncompatibleValuesPrunedFromBothSides)
{
    AdjacencyConstraint constraint(0, 1, makeTable());
    std::vector<Domain> wave{
        Domain::singleton(0, 3), Domain(3)};

    EXPECT_TRUE(constraint.prune(wave));
    EXPECT_FALSE(wave[1].contains(2));
    EXPECT_TRUE(wave[1].contains(0));
    EXPECT_TRUE(wave[1].contains(1));
}

TEST(AdjacencyConstraintTest, NoCompatiblePartnerFails)
{
    AdjacencyConstraint constraint(0, 1, makeTable());
    std::vector<Domain> wave{
        Domain::singleton(0, 3), Domain::singleton(2, 3)};

    EXPECT_FALSE(constraint.prune(wave));
}
