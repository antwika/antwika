#include <gtest/gtest.h>

#include <optional>
#include <vector>

#include "antwika/game/MenuState.hpp"

using antwika::game::entriesFor;
using antwika::game::kMenuEntries;
using antwika::game::kMenuEntryCount;
using antwika::game::kMenuLanguageCount;
using antwika::game::kMenuLanguages;
using antwika::game::leavesMenu;
using antwika::game::MenuEntry;
using antwika::game::menuEntryIndex;
using antwika::game::MenuLanguage;
using antwika::game::menuLanguageIndex;
using antwika::game::MenuState;

TEST(MenuStateTest, EntriesFor_LeavesResumeOutBeforeAGameHasBegun)
{
    const auto entries = entriesFor(false);

    ASSERT_EQ(kMenuEntryCount - 1, entries.size());
    EXPECT_EQ(MenuEntry::PlayGame, entries[0]);
    EXPECT_EQ(MenuEntry::LoadReplay, entries[1]);
    EXPECT_EQ(MenuEntry::SaveReplay, entries[2]);
}

TEST(MenuStateTest, EntriesFor_OffersResumeOnceAGameHasBegun)
{
    const auto entries = entriesFor(true);

    ASSERT_EQ(kMenuEntryCount, entries.size());
    EXPECT_EQ(MenuEntry::ResumeGame, entries[kMenuEntryCount - 1]);
}

TEST(MenuStateTest, LeavesMenu_KeepsTheMenuUpOnlyForSaving)
{
    EXPECT_TRUE(leavesMenu(MenuEntry::PlayGame));
    EXPECT_TRUE(leavesMenu(MenuEntry::LoadReplay));
    EXPECT_TRUE(leavesMenu(MenuEntry::ResumeGame));
    EXPECT_FALSE(leavesMenu(MenuEntry::SaveReplay));
}

// The tables everything else indexes are only right if these agree.
TEST(MenuStateTest, MenuEntryIndex_MatchesWhereTheEntryIsListed)
{
    for (std::size_t index = 0; index < kMenuEntryCount; ++index)
    {
        EXPECT_EQ(index, menuEntryIndex(kMenuEntries[index]));
    }
}

TEST(MenuStateTest, MenuLanguageIndex_MatchesWhereTheLanguageIsListed)
{
    for (std::size_t index = 0; index < kMenuLanguageCount; ++index)
    {
        EXPECT_EQ(index, menuLanguageIndex(kMenuLanguages[index]));
    }
}

TEST(MenuStateTest, Construct_StartsClosedOnAGameThatHasNotBegun)
{
    const MenuState state;

    EXPECT_FALSE(state.open);
    EXPECT_FALSE(state.gameBegun);
    EXPECT_EQ(MenuLanguage::English, state.language);
    EXPECT_EQ(std::nullopt, state.activated);
}

TEST(MenuStateTest, Compare_DiffersOnEveryField)
{
    const MenuState state;

    EXPECT_EQ(state, MenuState{});
    EXPECT_NE(state, (MenuState{.open = true}));
    EXPECT_NE(state, (MenuState{.gameBegun = true}));
    EXPECT_NE(
        state, (MenuState{.language = MenuLanguage::Swedish}));
    EXPECT_NE(
        state, (MenuState{.activated = MenuEntry::PlayGame}));
}
