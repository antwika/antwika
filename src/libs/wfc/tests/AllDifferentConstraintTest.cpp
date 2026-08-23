#include <gtest/gtest.h>

#include <vector>

#include <antwika/wfc/AllDifferentConstraint.hpp>
#include <antwika/wfc/Domain.hpp>

using antwika::wfc::AllDifferentConstraint;
using antwika::wfc::Domain;

TEST(AllDifferentConstraintTest, Cells_ReturnsTheConstructedIndices)
{
    AllDifferentConstraint constraint({0, 2, 4});
    std::vector<std::size_t> cells(
        constraint.getCells().begin(), constraint.getCells().end());
    EXPECT_EQ(cells, (std::vector<std::size_t>{0, 2, 4}));
}

TEST(AllDifferentConstraintTest, Prune_RemovesASingletonFromOthers)
{
    AllDifferentConstraint constraint({0, 1, 2});
    std::vector<Domain> waveDomains{
        Domain::createSingleton(1, 3), Domain(3), Domain(3)};

    EXPECT_TRUE(constraint.prune(waveDomains));

    EXPECT_TRUE(waveDomains[0].isSingleton());
    EXPECT_FALSE(waveDomains[1].contains(1));
    EXPECT_FALSE(waveDomains[2].contains(1));
    EXPECT_TRUE(waveDomains[1].contains(0));
    EXPECT_TRUE(waveDomains[1].contains(2));
}

TEST(AllDifferentConstraintTest, Prune_FailsOnContradictoryInput)
{
    AllDifferentConstraint constraint({0, 1});
    std::vector<Domain> waveDomains{
        Domain::createSingleton(1, 2), Domain::createSingleton(1, 2)};

    EXPECT_FALSE(constraint.prune(waveDomains));
}

TEST(AllDifferentConstraintTest, Prune_ReturnsTrueWhenNothingChanges)
{
    AllDifferentConstraint constraint({0, 1});
    std::vector<Domain> waveDomains{Domain(3), Domain(3)};

    EXPECT_TRUE(constraint.prune(waveDomains));
    EXPECT_EQ(waveDomains[0].getCount(), 3U);
    EXPECT_EQ(waveDomains[1].getCount(), 3U);
}
