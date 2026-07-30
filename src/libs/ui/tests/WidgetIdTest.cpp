#include <gtest/gtest.h>

#include "antwika/ui/WidgetId.hpp"

using antwika::ui::assertDistinct;
using antwika::ui::kNoWidget;
using antwika::ui::WidgetId;

namespace
{
    constexpr WidgetId kFirst{1};
    constexpr WidgetId kSecond{2};
    constexpr WidgetId kThird{3};
} // namespace

// Every case is asserted in a constant expression, as intended.
// So nothing here is a runtime call, and none of it can be covered.

TEST(WidgetIdTest, AssertDistinct_AcceptsIdsThatAllDiffer)
{
    static_assert(assertDistinct(kFirst, kSecond, kThird));

    SUCCEED();
}

TEST(WidgetIdTest, AssertDistinct_RejectsARepeatedId)
{
    static_assert(!assertDistinct(kFirst, kSecond, kFirst));

    SUCCEED();
}

TEST(WidgetIdTest, AssertDistinct_RejectsAdjacentDuplicates)
{
    static_assert(!assertDistinct(kFirst, kFirst));

    SUCCEED();
}

TEST(WidgetIdTest, AssertDistinct_TreatsNoWidgetAsAnOrdinaryValue)
{
    static_assert(assertDistinct(kNoWidget, kFirst));
    static_assert(!assertDistinct(kNoWidget, kNoWidget));

    SUCCEED();
}

TEST(WidgetIdTest, AssertDistinct_IsVacuouslyTrueForFewerThanTwoIds)
{
    static_assert(assertDistinct());
    static_assert(assertDistinct(kFirst));

    SUCCEED();
}
