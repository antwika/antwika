#include <cstddef>

#include <gtest/gtest.h>

#include <optional>
#include <set>

#include <antwika/i18n/Locale.hpp>
#include <antwika/ui/DropdownSpec.hpp>
#include <antwika/ui/TextFieldSpec.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/ui_demo/DemoState.hpp"
#include "antwika/ui_demo/MessageId.hpp"

using antwika::ui_demo::accentNameId;
using antwika::ui_demo::DemoState;
using antwika::ui_demo::kAccentCount;
using antwika::ui_demo::kShowcaseCount;
using antwika::ui_demo::Showcase;
using antwika::ui_demo::showcaseNameId;

namespace
{
    TEST(DemoStateTest, Select_ShowsThePageAnIndexNames)
    {
        DemoState state;
        EXPECT_EQ(state.showcase(), Showcase::Labels);
        EXPECT_EQ(state.selected(), 0U);

        state.select(static_cast<std::size_t>(Showcase::Dropdown));
        EXPECT_EQ(state.showcase(), Showcase::Dropdown);
        EXPECT_EQ(
            state.selected(),
            static_cast<std::size_t>(Showcase::Dropdown));
    }

    TEST(DemoStateTest, Select_LeavesThePageWhenNoPageHasThatIndex)
    {
        DemoState state;
        state.select(static_cast<std::size_t>(Showcase::Theme));

        state.select(kShowcaseCount);
        EXPECT_EQ(state.showcase(), Showcase::Theme);
    }

    TEST(DemoStateTest, SetPickerOpen_HoldsTheListsOwnBit)
    {
        DemoState state;
        EXPECT_FALSE(state.pickerOpen());

        state.setPickerOpen(true);
        EXPECT_TRUE(state.pickerOpen());
    }

    TEST(DemoStateTest, SelectAccent_TakesAnIndexTheListHas)
    {
        DemoState state;
        EXPECT_EQ(state.accent(), antwika::ui::kNoOption);

        state.selectAccent(1);
        EXPECT_EQ(state.accent(), 1U);
    }

    TEST(DemoStateTest, SelectAccent_SelectsNothingOutsideTheList)
    {
        DemoState state;
        state.selectAccent(1);

        state.selectAccent(kAccentCount);
        EXPECT_EQ(state.accent(), antwika::ui::kNoOption);
    }

    TEST(DemoStateTest, SetAccentOpen_HoldsTheSecondListsBit)
    {
        DemoState state;
        EXPECT_FALSE(state.accentOpen());

        state.setAccentOpen(true);
        EXPECT_TRUE(state.accentOpen());
    }

    TEST(DemoStateTest, SetText_TakesWhatAnEditReported)
    {
        DemoState state;
        EXPECT_TRUE(state.text().empty());
        EXPECT_EQ(state.caret(), antwika::ui::kCaretAtEnd);

        state.setText("abc", 2);
        EXPECT_EQ(state.text(), "abc");
        EXPECT_EQ(state.caret(), 2U);
    }

    TEST(DemoStateTest, SetFocus_HoldsWhatTheLastFrameHandedBack)
    {
        DemoState state;
        EXPECT_EQ(state.focus(), antwika::ui::kNoWidget);

        state.setFocus(antwika::ui::WidgetId{7});
        EXPECT_EQ(state.focus(), antwika::ui::WidgetId{7});
    }

    TEST(DemoStateTest, CountClick_CountsUpAndResetsToZero)
    {
        DemoState state;
        EXPECT_EQ(state.clicks(), 0U);

        state.countClick();
        state.countClick();
        EXPECT_EQ(state.clicks(), 2U);

        state.resetClicks();
        EXPECT_EQ(state.clicks(), 0U);
    }

    TEST(DemoStateTest, SetMessage_HoldsWhatWasSaid)
    {
        DemoState state;
        EXPECT_FALSE(state.message().has_value());

        // An id and a datum, never a sentence.
        // A sentence here would be the locale inside the state.
        state.setMessage(
            {.id = antwika::ui_demo::MessageId::Submitted,
             .datum = "ok",
             .argId = std::nullopt});

        ASSERT_TRUE(state.message().has_value());
        EXPECT_EQ(
            state.message()->id,
            antwika::ui_demo::MessageId::Submitted);
        EXPECT_EQ(state.message()->datum, "ok");
        EXPECT_FALSE(state.message()->argId.has_value());
    }

    // Every page and every accent has an id, and no two share one.
    // Which is the only thing left to check here now the words moved.
    TEST(ShowcaseTest, ShowcaseNameId_NamesEveryPageThePickerLists)
    {
        std::set<antwika::ui_demo::MessageId> ids;

        for (std::size_t index = 0; index < kShowcaseCount; ++index)
        {
            ids.insert(showcaseNameId(static_cast<Showcase>(index)));
        }

        EXPECT_EQ(ids.size(), kShowcaseCount);
    }

    TEST(ShowcaseTest, AccentNameId_NamesEveryAccentAndWrapsOnOneMore)
    {
        std::set<antwika::ui_demo::MessageId> ids;

        for (std::size_t index = 0; index < kAccentCount; ++index)
        {
            ids.insert(accentNameId(index));
        }

        EXPECT_EQ(ids.size(), kAccentCount);
        EXPECT_EQ(accentNameId(kAccentCount), accentNameId(0));
    }
} // namespace
