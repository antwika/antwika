#include <gtest/gtest.h>

#include <algorithm>
#include <set>
#include <string_view>

#include "antwika/editor/ui/EditorLook.hpp"
#include "antwika/editor/ui/MenuBar.hpp"

#include "antwika/editor/ui/WidgetIds.hpp"

namespace
{

    using antwika::editor::isToggle;
    using antwika::editor::itemAt;
    using antwika::editor::getItemName;
    using antwika::editor::itemNamesOf;
    using antwika::editor::itemsOf;
    using antwika::editor::getFirstItemWidget;
    using antwika::editor::kMaxMenuLines;
    using antwika::editor::kEveryMenu;
    using antwika::editor::Menu;
    using antwika::editor::MenuItem;
    using antwika::editor::getMenuName;
    using antwika::editor::getMenuWidget;

    TEST(MenuBarTest, ItemsOf_NamesEveryLineOnceAndOnlyOnce)
    {
        std::set<MenuItem> seenItems;

        for (const auto menu : kEveryMenu)
        {
            for (const auto item : itemsOf(menu))
            {
                EXPECT_TRUE(seenItems.insert(item).second);
                EXPECT_FALSE(getItemName(item).empty());
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

    TEST(MenuBarTest, ItemsOf_KeepsTheGameSettingsApartFromTheEditorsOwn)
    {
        const auto game = itemsOf(Menu::Game);
        const auto view = itemsOf(Menu::View);

        EXPECT_NE(
            std::ranges::find(game, MenuItem::GameLighting), game.end());
        EXPECT_NE(
            std::ranges::find(game, MenuItem::Corners), game.end());
        EXPECT_EQ(
            std::ranges::find(game, MenuItem::EditorLighting), game.end());

        EXPECT_NE(
            std::ranges::find(view, MenuItem::EditorLighting), view.end());
        EXPECT_EQ(
            std::ranges::find(view, MenuItem::GameLighting), view.end());

        for (const auto item : view)
        {
            EXPECT_TRUE(isToggle(item));
        }
    }

    TEST(MenuBarTest, ItemsOf_HangsBothLightingLinesOffTheSameName)
    {
        EXPECT_EQ(
            getItemName(MenuItem::GameLighting),
            getItemName(MenuItem::EditorLighting));
    }

    TEST(MenuBarTest, EveryMenu_StandsOnTheBar)
    {
        EXPECT_EQ(kEveryMenu.size(), 4U);
        EXPECT_EQ(getMenuName(Menu::Game), "Game");
        EXPECT_EQ(getMenuName(Menu::View), "View");
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
                EXPECT_EQ(names[index], getItemName(items[index]));
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
            EXPECT_NE(getMenuWidget(menu), antwika::widget::kNoWidget);
            EXPECT_TRUE(seenWidgets.insert(getMenuWidget(menu)).second);
        }
    }

    TEST(MenuBarTest, ItemWidgets_LeavesRoomForEveryLineOfAList)
    {
        std::set<antwika::widget::WidgetId> seenWidgets;

        for (const auto menu : kEveryMenu)
        {
            const auto first =
                static_cast<std::uint64_t>(getFirstItemWidget(menu));

            for (std::size_t index = 0; index < itemsOf(menu).size(); ++index)
            {
                const auto widget = antwika::widget::WidgetId{
                    first + static_cast<std::uint64_t>(index)};

                EXPECT_TRUE(seenWidgets.insert(widget).second);
                EXPECT_NE(widget, getMenuWidget(menu));
            }
        }
    }

    TEST(MenuBarTest, IsToggle_HoldsOnlyForWhatTurnsOnAndOff)
    {
        EXPECT_TRUE(isToggle(MenuItem::Grid));
        EXPECT_TRUE(isToggle(MenuItem::Marker));
        EXPECT_TRUE(isToggle(MenuItem::RuleLines));
        EXPECT_TRUE(isToggle(MenuItem::FreeLook));
        EXPECT_TRUE(isToggle(MenuItem::GameLighting));
        EXPECT_TRUE(isToggle(MenuItem::EditorLighting));
        EXPECT_TRUE(isToggle(MenuItem::Corners));
        EXPECT_FALSE(isToggle(MenuItem::New));
        EXPECT_FALSE(isToggle(MenuItem::Save));
        EXPECT_FALSE(isToggle(MenuItem::Load));
        EXPECT_FALSE(isToggle(MenuItem::Keys));
        EXPECT_FALSE(isToggle(MenuItem::Quit));
    }

    TEST(MenuBarTest, MenuName_NamesEveryMenu)
    {
        for (const auto menu : kEveryMenu)
        {
            EXPECT_FALSE(getMenuName(menu).empty());
        }
    }

}
