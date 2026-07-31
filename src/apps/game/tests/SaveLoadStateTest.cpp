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

    TEST(SaveLoadStateTest, AnEmptyDirectorySelectsNothing)
    {
        const SaveLoadState state;

        EXPECT_TRUE(state.options().empty());
        EXPECT_EQ(state.selected(), kNoOption);
        EXPECT_TRUE(state.selectedName().empty());
    }

    // Pressing Load without touching the list is the obvious thing.
    TEST(SaveLoadStateTest, TheFirstSaveIsSelectedToBeginWith)
    {
        const SaveLoadState state({"beta", "alpha"});

        ASSERT_EQ(state.options().size(), 2U);
        EXPECT_EQ(state.options()[0], "alpha");
        EXPECT_EQ(state.selectedName(), "alpha");
    }

    TEST(SaveLoadStateTest, SelectingOutsideTheListSelectsNothing)
    {
        SaveLoadState state({"alpha", "beta"});

        state.select(1);
        EXPECT_EQ(state.selectedName(), "beta");

        state.select(9);
        EXPECT_EQ(state.selected(), kNoOption);
        EXPECT_TRUE(state.selectedName().empty());
    }

    TEST(SaveLoadStateTest, AddingASaveKeepsTheListSortedAndSelectsIt)
    {
        SaveLoadState state({"beta"});

        state.add("alpha");

        ASSERT_EQ(state.options().size(), 2U);
        EXPECT_EQ(state.options()[0], "alpha");
        EXPECT_EQ(state.selected(), 0U);
        EXPECT_EQ(state.selectedName(), "alpha");
    }

    // Saving over a name already there adds nothing and selects it.
    TEST(SaveLoadStateTest, AddingANameAlreadyThereOnlySelectsIt)
    {
        SaveLoadState state({"alpha", "beta"});

        state.add("beta");

        EXPECT_EQ(state.options().size(), 2U);
        EXPECT_EQ(state.selected(), 1U);
    }

    TEST(SaveLoadStateTest, TheListOpensAndCloses)
    {
        SaveLoadState state;

        EXPECT_FALSE(state.listOpen());
        state.setListOpen(true);
        EXPECT_TRUE(state.listOpen());
    }

    TEST(SaveLoadStateTest, TheFieldTheCaretAndTheFocusAreAllTheCallers)
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

    TEST(SaveLoadStateTest, TheMessageIsWhateverWasLastSaid)
    {
        SaveLoadState state;

        EXPECT_TRUE(state.message().empty());
        state.setMessage("Saved town");
        EXPECT_EQ(state.message(), "Saved town");
    }

    // A view per name, over the names themselves.
    // Inserting may move every buffer, so they are rebuilt whole.
    TEST(SaveLoadStateTest, TheOptionsStayValidAfterAnInsert)
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

} // namespace
