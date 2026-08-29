#include <gtest/gtest.h>

#include <vector>

#include <antwika/wfc/Domain.hpp>
#include <antwika/wfc/WfcError.hpp>

using antwika::wfc::Domain;
using antwika::wfc::WfcError;

TEST(DomainTest, Ctor_SetsEveryBit)
{
    Domain domain(4);
    EXPECT_EQ(domain.getCount(), 4U);
    for (std::size_t v = 0; v < 4; ++v)
    {
        EXPECT_TRUE(domain.contains(v));
    }
    EXPECT_FALSE(domain.isEmpty());
    EXPECT_FALSE(domain.isSingleton());
}

TEST(DomainTest, Singleton_SetsExactlyOneBit)
{
    Domain domain = Domain::createSingleton(2, 4);
    EXPECT_EQ(domain.getCount(), 1U);
    EXPECT_TRUE(domain.contains(2));
    EXPECT_FALSE(domain.contains(0));
    EXPECT_TRUE(domain.isSingleton());
    EXPECT_EQ(domain.getSingleValue(), 2U);
}

TEST(DomainTest, SingleValue_ThrowsWhenMoreThanOneRemains)
{
    Domain domain(4);
    EXPECT_THROW(
        static_cast<void>(domain.getSingleValue()), WfcError);
}

TEST(DomainTest, SingleValue_ThrowsOnAnEmptyDomain)
{
    Domain domain(2);
    domain.remove(0);
    domain.remove(1);
    EXPECT_THROW(
        static_cast<void>(domain.getSingleValue()), WfcError);
}

TEST(DomainTest, Add_RestoresExactlyTheRemovedBit)
{
    Domain domain(3);
    domain.remove(1);
    EXPECT_FALSE(domain.contains(1));
    EXPECT_EQ(domain.getCount(), 2U);

    domain.add(1);
    EXPECT_TRUE(domain.contains(1));
    EXPECT_EQ(domain.getCount(), 3U);
}

TEST(DomainTest, RestrictTo_LeavesOnlyThatValue)
{
    Domain domain(5);
    domain.restrictTo(3);
    EXPECT_TRUE(domain.isSingleton());
    EXPECT_EQ(domain.getSingleValue(), 3U);
}

TEST(DomainTest, Remove_EmptiesTheDomainEventually)
{
    Domain domain(2);
    domain.remove(0);
    domain.remove(1);
    EXPECT_TRUE(domain.isEmpty());
    EXPECT_EQ(domain.getCount(), 0U);
    EXPECT_FALSE(domain.isSingleton());
}

TEST(DomainTest, Remove_CountsTheSameValueOnce)
{
    Domain domain(3);
    domain.remove(1);
    domain.remove(1);

    EXPECT_EQ(domain.getCount(), 2U);
    EXPECT_FALSE(domain.contains(1));
}

TEST(DomainTest, Add_ChangesNothingForAPresentValue)
{
    Domain domain(3);
    domain.add(1);

    EXPECT_EQ(domain.getCount(), 3U);
    EXPECT_EQ(domain, Domain(3));
}

TEST(DomainTest, RestrictTo_EmptiesTheDomainForARemovedValue)
{
    Domain domain(3);
    domain.remove(2);
    domain.restrictTo(2);

    EXPECT_TRUE(domain.isEmpty());
    EXPECT_EQ(domain.getCount(), 0U);
    EXPECT_FALSE(domain.contains(2));
}

TEST(DomainTest, Values_IterateAscending)
{
    Domain domain(6);
    domain.remove(0);
    domain.remove(3);

    std::vector<std::size_t> values(domain.begin(), domain.end());
    std::vector<std::size_t> expectedValues{1, 2, 4, 5};
    EXPECT_EQ(values, expectedValues);
}

TEST(DomainTest, OperatorEquals_ComparesBits)
{
    Domain domain(3);
    Domain otherDomain(3);
    EXPECT_EQ(domain, otherDomain);

    otherDomain.remove(1);
    EXPECT_NE(domain, otherDomain);

    domain.remove(1);
    EXPECT_EQ(domain, otherDomain);
}

TEST(DomainTest, AlphabetSize_IsPreserved)
{
    Domain domain(7);
    EXPECT_EQ(domain.getAlphabetSize(), 7U);
}

TEST(DomainTest, OperatorIncrement_ReturnsThePriorPosition)
{
    Domain domain(3);
    Domain::const_iterator it = domain.begin();
    Domain::const_iterator prior = it++;
    EXPECT_EQ(*prior, 0U);
    EXPECT_EQ(*it, 1U);
}

TEST(DomainTest, OperatorEquals_ComparesIteratorPosition)
{
    Domain domain(3);
    Domain::const_iterator first = domain.begin();
    Domain::const_iterator second = domain.begin();
    EXPECT_TRUE(first == second);

    ++second;
    EXPECT_FALSE(first == second);
}

TEST(DomainTest, OperatorEquals_ComparesTheIteratorsDomain)
{
    Domain domain(3);
    Domain otherDomain(3);
    EXPECT_FALSE(domain.begin() == otherDomain.begin());
}

TEST(DomainTest, Remove_IgnoresAnOutOfRangeValue)
{
    Domain domain(3);
    EXPECT_FALSE(domain.contains(5));

    domain.remove(5);
    EXPECT_EQ(domain.getCount(), 3U);

    domain.add(5);
    EXPECT_EQ(domain.getCount(), 3U);
}

TEST(DomainTest, Add_IgnoresTheValueOneStepPastTheAlphabet)
{
    Domain domain(3);

    domain.add(3);

    EXPECT_EQ(domain.getCount(), 3U);
    EXPECT_FALSE(domain.contains(3));
    EXPECT_EQ(domain.getAlphabetSize(), 3U);
}

TEST(DomainTest, RestrictTo_ThrowsOnAnOutOfRangeValue)
{
    Domain domain(3);
    EXPECT_THROW(domain.restrictTo(5), WfcError);
    EXPECT_EQ(domain, Domain(3));
}

TEST(DomainTest, RestrictTo_ThrowsOnTheValueOneStepPastTheAlphabet)
{
    Domain domain(3);
    EXPECT_THROW(domain.restrictTo(3), WfcError);
    EXPECT_EQ(domain, Domain(3));
}
