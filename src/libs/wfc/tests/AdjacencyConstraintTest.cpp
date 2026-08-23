#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include <antwika/wfc/AdjacencyConstraint.hpp>
#include <antwika/wfc/CompatibilityTable.hpp>
#include <antwika/wfc/Domain.hpp>
#include <antwika/wfc/WfcError.hpp>

using antwika::wfc::AdjacencyConstraint;
using antwika::wfc::CompatibilityTable;
using antwika::wfc::Domain;
using antwika::wfc::WfcError;

namespace
{
    CompatibilityTable createTable()
    {
        CompatibilityTable table(3);
        table.set(0, 2, false);
        table.set(2, 0, false);
        return table;
    }

    CompatibilityTable createTablePruningAValueFromEachSide()
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
    AdjacencyConstraint constraint(3, 7, createTable());
    std::vector<std::size_t> cells(
        constraint.getCells().begin(), constraint.getCells().end());
    EXPECT_EQ(cells, (std::vector<std::size_t>{3, 7}));
}

TEST(AdjacencyConstraintTest, Prune_RemovesIncompatibleValuesBothSides)
{
    AdjacencyConstraint constraint(0, 1, createTablePruningAValueFromEachSide());
    std::vector<Domain> waveDomains{Domain(3), Domain(3)};

    EXPECT_TRUE(constraint.prune(waveDomains));
    EXPECT_TRUE(waveDomains[0].contains(0));
    EXPECT_TRUE(waveDomains[0].contains(1));
    EXPECT_FALSE(waveDomains[0].contains(2));
    EXPECT_FALSE(waveDomains[1].contains(0));
    EXPECT_TRUE(waveDomains[1].contains(1));
    EXPECT_TRUE(waveDomains[1].contains(2));
}

TEST(AdjacencyConstraintTest, Prune_FailsWithNoCompatiblePartner)
{
    AdjacencyConstraint constraint(0, 1, createTable());
    std::vector<Domain> waveDomains{
        Domain::createSingleton(0, 3), Domain::createSingleton(2, 3)};

    EXPECT_FALSE(constraint.prune(waveDomains));
}

TEST(
    AdjacencyConstraintTest,
    Prune_IsSafeWhenTheTableIsSmaller)
{
    CompatibilityTable table(2);
    AdjacencyConstraint constraint(0, 1, table);
    std::vector<Domain> waveDomains{Domain(4), Domain(4)};

    EXPECT_TRUE(constraint.prune(waveDomains));
    EXPECT_FALSE(waveDomains[0].contains(2));
    EXPECT_FALSE(waveDomains[0].contains(3));
    EXPECT_TRUE(waveDomains[0].contains(0));
    EXPECT_TRUE(waveDomains[0].contains(1));
    EXPECT_FALSE(waveDomains[1].contains(2));
    EXPECT_FALSE(waveDomains[1].contains(3));
    EXPECT_TRUE(waveDomains[1].contains(0));
    EXPECT_TRUE(waveDomains[1].contains(1));
}

TEST(AdjacencyConstraintTest, Prune_ReadsATableSharedWithOtherConstraints)
{
    const auto sharedTable =
        std::make_shared<const CompatibilityTable>(createTable());
    AdjacencyConstraint firstConstraint(0, 1, sharedTable);
    AdjacencyConstraint secondConstraint(1, 2, sharedTable);
    std::vector<Domain> waveDomains{
        Domain::createSingleton(0, 3), Domain(3), Domain::createSingleton(0, 3)};

    EXPECT_TRUE(firstConstraint.prune(waveDomains));
    EXPECT_TRUE(secondConstraint.prune(waveDomains));
    EXPECT_FALSE(waveDomains[1].contains(2));
    EXPECT_EQ(sharedTable.use_count(), 3);
}

TEST(AdjacencyConstraintTest, Ctor_RejectsATableThatIsNothing)
{
    EXPECT_THROW(
        AdjacencyConstraint(0, 1, std::shared_ptr<const CompatibilityTable>{}),
        WfcError);
}
