#include <antwika/wfc/ConstraintRefs.hpp>

#include <array>
#include <deque>
#include <vector>

#include <gtest/gtest.h>

#include <antwika/wfc/AdjacencyConstraint.hpp>
#include <antwika/wfc/CompatibilityTable.hpp>
#include <antwika/wfc/IConstraint.hpp>

using antwika::wfc::AdjacencyConstraint;
using antwika::wfc::CompatibilityTable;
using antwika::wfc::IConstraint;
using antwika::wfc::referencesTo;

namespace
{
    AdjacencyConstraint between(std::size_t left, std::size_t right)
    {
        return AdjacencyConstraint(left, right, CompatibilityTable(2));
    }
} // namespace

TEST(ConstraintRefsTest, ReferencesEachElementInOrder)
{
    const std::vector<AdjacencyConstraint> constraints{
        between(0, 1), between(1, 2), between(2, 3)};

    const auto refs = referencesTo(constraints);

    ASSERT_EQ(refs.size(), constraints.size());
    for (std::size_t i = 0; i < constraints.size(); ++i)
    {
        EXPECT_EQ(&refs[i].get(), &constraints[i]);
    }
}

// Nothing is copied, so the identity above is the whole contract.
// A reference into a container that reallocated would be dangling.
// So the doc's "must outlive" is the caller's, not this function's.
TEST(ConstraintRefsTest, ReferencesRatherThanCopies)
{
    const std::vector<AdjacencyConstraint> constraints{between(0, 1)};

    const auto refs = referencesTo(constraints);

    const IConstraint &referenced = refs.front().get();
    EXPECT_EQ(&referenced, &constraints.front());
}

TEST(ConstraintRefsTest, AnEmptyRangeYieldsAnEmptyVector)
{
    const std::vector<AdjacencyConstraint> none;

    EXPECT_TRUE(referencesTo(none).empty());
}

// Any range will do, since Solver only ever sees the vector this makes.
// A caller holding an array or a deque builds no vector to hand over.
TEST(ConstraintRefsTest, AcceptsAnyRangeOfConstraints)
{
    const std::array<AdjacencyConstraint, 2> asArray{
        between(0, 1), between(1, 2)};
    const std::deque<AdjacencyConstraint> asDeque{
        between(0, 1), between(1, 2)};

    EXPECT_EQ(referencesTo(asArray).size(), 2U);
    EXPECT_EQ(&referencesTo(asDeque).front().get(), &asDeque.front());
}
