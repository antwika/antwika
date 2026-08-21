#include <gtest/gtest.h>

#include "antwika/ui/WidgetId.hpp"

using antwika::ui::allDistinct;
using antwika::ui::kNoWidget;
using antwika::ui::WidgetId;

namespace
{
    constexpr WidgetId kFirstWidget{1};
    constexpr WidgetId kSecondWidget{2};
    constexpr WidgetId kThirdWidget{3};
}

TEST(WidgetIdTest, AssertDistinct_AcceptsIdsThatAllDiffer)
{
    static_assert(allDistinct(kFirstWidget, kSecondWidget, kThirdWidget));

    SUCCEED();
}

TEST(WidgetIdTest, AssertDistinct_RejectsARepeatedId)
{
    static_assert(!allDistinct(kFirstWidget, kSecondWidget, kFirstWidget));

    SUCCEED();
}

TEST(WidgetIdTest, AssertDistinct_RejectsAdjacentDuplicates)
{
    static_assert(!allDistinct(kFirstWidget, kFirstWidget));

    SUCCEED();
}

TEST(WidgetIdTest, AssertDistinct_TreatsNoWidgetAsAnOrdinaryValue)
{
    static_assert(allDistinct(kNoWidget, kFirstWidget));
    static_assert(!allDistinct(kNoWidget, kNoWidget));

    SUCCEED();
}

TEST(WidgetIdTest, AssertDistinct_IsVacuouslyTrueForFewerThanTwoIds)
{
    static_assert(allDistinct());
    static_assert(allDistinct(kFirstWidget));

    SUCCEED();
}
