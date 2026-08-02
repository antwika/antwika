#include "antwika/pattern/Controls.hpp"

#include <gtest/gtest.h>

#include "antwika/pattern/ParamId.hpp"
#include "antwika/pattern/ParamValue.hpp"

using antwika::pattern::Control;
using antwika::pattern::Controls;
using antwika::pattern::kNoParam;
using antwika::pattern::ParamId;
using antwika::pattern::ParamValue;

namespace
{
    constexpr ParamId kPitch{1};
    constexpr ParamId kGain{2};
    constexpr ParamId kPan{3};
} // namespace

TEST(ControlsTest, StartsCarryingNothing)
{
    const Controls empty;

    EXPECT_TRUE(empty.empty());
    EXPECT_EQ(empty.size(), 0U);
    EXPECT_FALSE(empty.get(kPitch).has_value());
}

TEST(ControlsTest, CarriesOneValueFromItsConstructor)
{
    const Controls one(kPitch, ParamValue(60));

    EXPECT_EQ(one.size(), 1U);
    EXPECT_EQ(one.get(kPitch), ParamValue(60));
}

TEST(ControlsTest, ReplacesAValueAlreadyNamed)
{
    Controls values(kPitch, ParamValue(60));
    values.set(kPitch, ParamValue(62));

    EXPECT_EQ(values.size(), 1U);
    EXPECT_EQ(values.get(kPitch), ParamValue(62));
}

TEST(ControlsTest, DoesNotFindAnIdItWasNeverGiven)
{
    const Controls values(kPitch, ParamValue(60));

    EXPECT_FALSE(values.get(kGain).has_value());
    EXPECT_FALSE(values.get(kNoParam).has_value());
}

// Sorted by id, so equality ignores the order they were set in.
// Iteration is the same on every run.
TEST(ControlsTest, KeepsItsValuesInIdOrderHoweverTheyArrived)
{
    Controls forwards;
    forwards.set(kPitch, ParamValue(60));
    forwards.set(kGain, ParamValue(1, 2));
    forwards.set(kPan, ParamValue(0));

    Controls backwards;
    backwards.set(kPan, ParamValue(0));
    backwards.set(kGain, ParamValue(1, 2));
    backwards.set(kPitch, ParamValue(60));

    EXPECT_EQ(forwards, backwards);

    ASSERT_EQ(forwards.all().size(), 3U);
    EXPECT_EQ(forwards.all()[0].id, kPitch);
    EXPECT_EQ(forwards.all()[1].id, kGain);
    EXPECT_EQ(forwards.all()[2].id, kPan);
}

// A gain applied to a pattern overrides the one it already carried.
// The direction is what makes that true rather than the reverse.
TEST(ControlsTest, TheOtherSideWinsWhenTwoSetsAreCombined)
{
    const Controls base(kGain, ParamValue(1));
    const Controls over(kGain, ParamValue(1, 2));

    EXPECT_EQ(base.mergedWith(over).get(kGain), ParamValue(1, 2));
    EXPECT_EQ(over.mergedWith(base).get(kGain), ParamValue(1));
}

TEST(ControlsTest, CombiningKeepsWhatOnlyOneSideCarried)
{
    const Controls base(kPitch, ParamValue(60));
    const Controls over(kGain, ParamValue(1, 2));

    const auto combined = base.mergedWith(over);

    EXPECT_EQ(combined.size(), 2U);
    EXPECT_EQ(combined.get(kPitch), ParamValue(60));
    EXPECT_EQ(combined.get(kGain), ParamValue(1, 2));
}

TEST(ControlsTest, ComparesOnWhatItCarries)
{
    const Controls one(kPitch, ParamValue(60));

    EXPECT_EQ(one, Controls(kPitch, ParamValue(60)));
    EXPECT_NE(one, Controls(kPitch, ParamValue(62)));
    EXPECT_NE(one, Controls(kGain, ParamValue(60)));
    EXPECT_NE(one, Controls{});
}

TEST(ControlsTest, AControlComparesOnBothItsFields)
{
    const Control entry{.id = kPitch, .value = ParamValue{}};
    const Control other{.id = kGain, .value = ParamValue{}};
    const Control louder{.id = kPitch, .value = ParamValue(1)};

    EXPECT_EQ(entry, entry);
    EXPECT_NE(entry, other);
    EXPECT_NE(entry, louder);
}
