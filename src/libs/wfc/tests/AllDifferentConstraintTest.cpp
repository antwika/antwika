#include <antwika/wfc/AllDifferentConstraint.hpp>

#include <vector>

#include <gtest/gtest.h>

#include <antwika/wfc/Domain.hpp>

using antwika::wfc::AllDifferentConstraint;
using antwika::wfc::Domain;

TEST(AllDifferentConstraintTest, CellsReturnsConstructedIndices)
{
    AllDifferentConstraint constraint({0, 2, 4});
    std::vector<std::size_t> cells(
        constraint.cells().begin(), constraint.cells().end());
    EXPECT_EQ(cells, (std::vector<std::size_t>{0, 2, 4}));
}

TEST(AllDifferentConstraintTest, SingletonValueRemovedFromOthers)
{
    AllDifferentConstraint constraint({0, 1, 2});
    std::vector<Domain> wave{
        Domain::singleton(1, 3), Domain(3), Domain(3)};

    EXPECT_TRUE(constraint.prune(wave));

    EXPECT_TRUE(wave[0].isSingleton());
    EXPECT_FALSE(wave[1].contains(1));
    EXPECT_FALSE(wave[2].contains(1));
    EXPECT_TRUE(wave[1].contains(0));
    EXPECT_TRUE(wave[1].contains(2));
}

TEST(AllDifferentConstraintTest, AlreadyContradictoryInputFails)
{
    AllDifferentConstraint constraint({0, 1});
    std::vector<Domain> wave{
        Domain::singleton(1, 2), Domain::singleton(1, 2)};

    EXPECT_FALSE(constraint.prune(wave));
}

TEST(AllDifferentConstraintTest, NoChangeStillReturnsTrue)
{
    AllDifferentConstraint constraint({0, 1});
    std::vector<Domain> wave{Domain(3), Domain(3)};

    EXPECT_TRUE(constraint.prune(wave));
    EXPECT_EQ(wave[0].count(), 3U);
    EXPECT_EQ(wave[1].count(), 3U);
}
