#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <vector>

#include <antwika/ui/DropdownSpec.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/game/SaveLoadState.hpp"

namespace
{

    using antwika::game::SaveLoadState;
    using antwika::ui::kNoOption;
    using antwika::ui::kNoWidget;
    using antwika::ui::WidgetId;

    TEST(SaveLoadStateTest, Options_SelectNothingWhenEmpty)
    {
        const SaveLoadState state;

        EXPECT_TRUE(state.options().empty());
        EXPECT_EQ(state.selected(), kNoOption);
        EXPECT_TRUE(state.selectedName().empty());
    }

    TEST(SaveLoadStateTest, Options_SelectTheFirstSaveToBeginWith)
    {
        const SaveLoadState state({"beta", "alpha"});

        ASSERT_EQ(state.options().size(), 2U);
        EXPECT_EQ(state.options()[0], "alpha");
        EXPECT_EQ(state.selectedName(), "alpha");
    }

    TEST(SaveLoadStateTest, Select_SelectsNothingOutsideTheList)
    {
        SaveLoadState state({"alpha", "beta"});

        state.select(1);
        EXPECT_EQ(state.selectedName(), "beta");

        state.select(9);
        EXPECT_EQ(state.selected(), kNoOption);
        EXPECT_TRUE(state.selectedName().empty());
    }

    TEST(SaveLoadStateTest, Add_KeepsTheListSortedAndSelects)
    {
        SaveLoadState state({"beta"});

        state.add("alpha");

        ASSERT_EQ(state.options().size(), 2U);
        EXPECT_EQ(state.options()[0], "alpha");
        EXPECT_EQ(state.selected(), 0U);
        EXPECT_EQ(state.selectedName(), "alpha");
    }

    TEST(SaveLoadStateTest, Add_OnlySelectsAnExistingName)
    {
        SaveLoadState state({"alpha", "beta"});

        state.add("beta");

        EXPECT_EQ(state.options().size(), 2U);
        EXPECT_EQ(state.selected(), 1U);
    }

    TEST(SaveLoadStateTest, ListOpen_OpensAndCloses)
    {
        SaveLoadState state;

        EXPECT_FALSE(state.listOpen());
        state.setListOpen(true);
        EXPECT_TRUE(state.listOpen());
    }

    TEST(SaveLoadStateTest, Name_FieldCaretAndFocusAreTheCallers)
    {
        SaveLoadState state;

        EXPECT_TRUE(state.name().empty());
        EXPECT_EQ(state.focus(), kNoWidget);

        state.setName("town", 2);
        state.setFocus(WidgetId{7});

        EXPECT_EQ(state.name(), "town");
        EXPECT_EQ(state.caret(), 2U);
        EXPECT_EQ(state.focus(), WidgetId{7});
    }

    TEST(SaveLoadStateTest, Message_IsWhateverWasLastSaid)
    {
        SaveLoadState state;

        EXPECT_TRUE(state.message().empty());
        state.setMessage("Saved town");
        EXPECT_EQ(state.message(), "Saved town");
    }

    TEST(SaveLoadStateTest, Add_LeavesTheOptionsValid)
    {
        SaveLoadState state({"beta"});

        state.add("alpha");
        state.add("gamma");

        std::vector<std::string> seen;
        for (const auto option : state.options())
        {
            seen.emplace_back(option);
        }

        EXPECT_EQ(
            seen, (std::vector<std::string>{"alpha", "beta", "gamma"}));
    }

}
