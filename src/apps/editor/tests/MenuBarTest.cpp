#include <gtest/gtest.h>

#include <algorithm>
#include <set>
#include <string_view>

#include "antwika/editor/ui/EditorLook.hpp"
#include "antwika/editor/ui/MenuBar.hpp"

namespace
{

    using antwika::editor::isToggle;
    using antwika::editor::itemAt;
    using antwika::editor::itemName;
    using antwika::editor::itemNamesOf;
    using antwika::editor::itemsOf;
    using antwika::editor::firstItemWidget;
    using antwika::editor::kBarMenus;
    using antwika::editor::kMaxMenuLines;
    using antwika::editor::Menu;
    using antwika::editor::MenuItem;
    using antwika::editor::menuName;
    using antwika::editor::menuWidget;

    constexpr std::array kEveryMenu{
        Menu::File, Menu::Edit, Menu::View, Menu::Settings};

    TEST(MenuBarTest, ItemsOf_NamesEveryLineOnceAndOnlyOnce)
    {
        std::set<MenuItem> seenItems;

        for (const auto menu : kEveryMenu)
        {
            for (const auto item : itemsOf(menu))
            {
                EXPECT_TRUE(seenItems.insert(item).second);
                EXPECT_FALSE(itemName(item).empty());
            }
        }

        EXPECT_EQ(seenItems.size(), 20U);
    }

    TEST(MenuBarTest, ItemsOf_HoldsNoMoreLinesThanAMenuCanDraw)
    {
        for (const auto menu : kEveryMenu)
        {
            EXPECT_LE(itemsOf(menu).size(), kMaxMenuLines);
            EXPECT_LE(itemNamesOf(menu).size(), kMaxMenuLines);
        }
    }

    TEST(MenuBarTest, ItemsOf_HoldsGrowingABlockUnderEdit)
    {
        const auto edit = itemsOf(Menu::Edit);

        EXPECT_NE(
            std::ranges::find(edit, MenuItem::Grow), edit.end());
        EXPECT_FALSE(isToggle(MenuItem::Grow));
    }

    TEST(MenuBarTest, ItemNamesOf_NamesTheLinesInTheOrderTheyLie)
    {
        for (const auto menu : kEveryMenu)
        {
            const auto items = itemsOf(menu);
            const auto names = itemNamesOf(menu);

            ASSERT_EQ(items.size(), names.size());

            for (std::size_t index = 0; index < items.size(); ++index)
            {
                EXPECT_EQ(names[index], itemName(items[index]));
            }
        }
    }

    TEST(MenuBarTest, ItemAt_TakesTheLineAnOptionStandsFor)
    {
        for (const auto menu : kEveryMenu)
        {
            const auto items = itemsOf(menu);

            for (std::size_t index = 0; index < items.size(); ++index)
            {
                EXPECT_EQ(itemAt(menu, index), items[index]);
            }

            EXPECT_FALSE(
                itemAt(menu, items.size()).has_value());
        }
    }

    TEST(MenuBarTest, MenuWidget_GivesEveryMenuAWidgetOfItsOwn)
    {
        std::set<antwika::widget::WidgetId> seenWidgets;

        for (const auto menu : kEveryMenu)
        {
            EXPECT_NE(menuWidget(menu), antwika::widget::kNoWidget);
            EXPECT_TRUE(seenWidgets.insert(menuWidget(menu)).second);
        }
    }

    TEST(MenuBarTest, ItemWidgets_LeavesRoomForEveryLineOfAList)
    {
        std::set<antwika::widget::WidgetId> seenWidgets;

        for (const auto menu : kEveryMenu)
        {
            const auto first =
                static_cast<std::uint64_t>(firstItemWidget(menu));

            for (std::size_t index = 0; index < itemsOf(menu).size(); ++index)
            {
                const auto widget = antwika::widget::WidgetId{
                    first + static_cast<std::uint64_t>(index)};

                EXPECT_TRUE(seenWidgets.insert(widget).second);
                EXPECT_NE(widget, menuWidget(menu));
            }
        }
    }

    TEST(MenuBarTest, IsToggle_HoldsOnlyForWhatTurnsOnAndOff)
    {
        EXPECT_TRUE(isToggle(MenuItem::Grid));
        EXPECT_TRUE(isToggle(MenuItem::Marker));
        EXPECT_TRUE(isToggle(MenuItem::RuleLines));
        EXPECT_TRUE(isToggle(MenuItem::FreeLook));
        EXPECT_TRUE(isToggle(MenuItem::Lighting));
        EXPECT_TRUE(isToggle(MenuItem::Corners));
        EXPECT_FALSE(isToggle(MenuItem::New));
        EXPECT_FALSE(isToggle(MenuItem::Save));
        EXPECT_FALSE(isToggle(MenuItem::Load));
        EXPECT_FALSE(isToggle(MenuItem::Settings));
        EXPECT_FALSE(isToggle(MenuItem::Quit));
    }

    TEST(MenuBarTest, MenuName_NamesEveryMenu)
    {
        for (const auto menu : kEveryMenu)
        {
            EXPECT_FALSE(menuName(menu).empty());
        }
    }

}
