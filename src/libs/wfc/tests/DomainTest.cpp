#include <antwika/wfc/Domain.hpp>

#include <vector>

#include <gtest/gtest.h>

using antwika::wfc::Domain;

TEST(DomainTest, DefaultConstructionHasEveryBitSet)
{
    Domain domain(4);
    EXPECT_EQ(domain.count(), 4U);
    for (std::size_t v = 0; v < 4; ++v)
    {
        EXPECT_TRUE(domain.contains(v));
    }
    EXPECT_FALSE(domain.isEmpty());
    EXPECT_FALSE(domain.isSingleton());
}

TEST(DomainTest, SingletonHasExactlyOneBitSet)
{
    Domain domain = Domain::singleton(2, 4);
    EXPECT_EQ(domain.count(), 1U);
    EXPECT_TRUE(domain.contains(2));
    EXPECT_FALSE(domain.contains(0));
    EXPECT_TRUE(domain.isSingleton());
    EXPECT_EQ(domain.singleValue(), 2U);
}

TEST(DomainTest, RemoveThenAddRestoresExactlyThatBit)
{
    Domain domain(3);
    domain.remove(1);
    EXPECT_FALSE(domain.contains(1));
    EXPECT_EQ(domain.count(), 2U);

    domain.add(1);
    EXPECT_TRUE(domain.contains(1));
    EXPECT_EQ(domain.count(), 3U);
}

TEST(DomainTest, RestrictToLeavesOnlyThatValue)
{
    Domain domain(5);
    domain.restrictTo(3);
    EXPECT_TRUE(domain.isSingleton());
    EXPECT_EQ(domain.singleValue(), 3U);
}

TEST(DomainTest, RemovingEveryValueIsEmpty)
{
    Domain domain(2);
    domain.remove(0);
    domain.remove(1);
    EXPECT_TRUE(domain.isEmpty());
    EXPECT_EQ(domain.count(), 0U);
    EXPECT_FALSE(domain.isSingleton());
}

TEST(DomainTest, IterationIsAscending)
{
    Domain domain(6);
    domain.remove(0);
    domain.remove(3);

    std::vector<std::size_t> values(domain.begin(), domain.end());
    std::vector<std::size_t> expected{1, 2, 4, 5};
    EXPECT_EQ(values, expected);
}

TEST(DomainTest, EqualityComparesBits)
{
    Domain a(3);
    Domain b(3);
    EXPECT_EQ(a, b);

    b.remove(1);
    EXPECT_NE(a, b);

    a.remove(1);
    EXPECT_EQ(a, b);
}

TEST(DomainTest, AlphabetSizeIsPreserved)
{
    Domain domain(7);
    EXPECT_EQ(domain.alphabetSize(), 7U);
}

TEST(DomainTest, IteratorPostIncrementReturnsPriorPositionAndAdvances)
{
    Domain domain(3);
    Domain::const_iterator it = domain.begin();
    Domain::const_iterator prior = it++;
    EXPECT_EQ(*prior, 0U);
    EXPECT_EQ(*it, 1U);
}

TEST(DomainTest, IteratorEqualityComparesPosition)
{
    Domain domain(3);
    Domain::const_iterator first = domain.begin();
    Domain::const_iterator second = domain.begin();
    EXPECT_TRUE(first == second);

    ++second;
    EXPECT_FALSE(first == second);
}

TEST(DomainTest, IteratorEqualityComparesUnderlyingDomain)
{
    Domain a(3);
    Domain b(3);
    EXPECT_FALSE(a.begin() == b.begin());
}

TEST(DomainTest, OutOfRangeAccessIsIgnored)
{
    Domain domain(3);
    EXPECT_FALSE(domain.contains(5));

    domain.remove(5);
    EXPECT_EQ(domain.count(), 3U);

    domain.add(5);
    EXPECT_EQ(domain.count(), 3U);
}

TEST(DomainTest, RestrictToOutOfRangeValueIsIgnored)
{
    Domain domain(3);
    domain.restrictTo(5);
    EXPECT_EQ(domain, Domain(3));
}
