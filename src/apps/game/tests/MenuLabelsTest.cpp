#include <gtest/gtest.h>

#include <set>
#include <string>

#include "antwika/game/MenuLabels.hpp"
#include "antwika/game/MenuState.hpp"

using antwika::game::kMenuEntries;
using antwika::game::kMenuLanguages;
using antwika::game::labelFor;
using antwika::game::labelsFor;
using antwika::game::MenuEntry;
using antwika::game::MenuLabels;
using antwika::game::MenuLanguage;

// The defaults are the English catalogue.
// So there is no second copy of it to keep in step.
TEST(MenuLabelsTest, LabelsFor_AnswersTheDefaultsForEnglish)
{
    EXPECT_EQ(MenuLabels{}, labelsFor(MenuLanguage::English));
}

TEST(MenuLabelsTest, LabelsFor_TranslatesTheEntriesIntoSwedish)
{
    const auto swedish = labelsFor(MenuLanguage::Swedish);
    const MenuLabels english;

    EXPECT_NE(english, swedish);
    EXPECT_NE(english.playGame, swedish.playGame);
    EXPECT_NE(english.loadReplay, swedish.loadReplay);
    EXPECT_NE(english.saveReplay, swedish.saveReplay);
    EXPECT_NE(english.resumeGame, swedish.resumeGame);
    EXPECT_NE(english.language, swedish.language);
}

// A language is named in its own language.
// The selector then reads the same whichever one the menu is in.
TEST(MenuLabelsTest, LabelsFor_NamesEveryLanguageTheSameWay)
{
    const auto swedish = labelsFor(MenuLanguage::Swedish);
    const MenuLabels english;

    EXPECT_EQ(english.english, swedish.english);
    EXPECT_EQ(english.swedish, swedish.swedish);
}

// The font antwika::gfx draws with covers ASCII only.
TEST(MenuLabelsTest, LabelsFor_WritesEverySwedishLabelInAscii)
{
    const auto swedish = labelsFor(MenuLanguage::Swedish);

    for (const auto *label :
         {&swedish.title,
          &swedish.playGame,
          &swedish.loadReplay,
          &swedish.saveReplay,
          &swedish.resumeGame,
          &swedish.language,
          &swedish.english,
          &swedish.swedish})
    {
        for (const auto character : *label)
        {
            EXPECT_GE(character, ' ') << *label;
            EXPECT_LE(character, '~') << *label;
        }
    }
}

TEST(MenuLabelsTest, LabelFor_NamesEveryEntryDifferently)
{
    const MenuLabels labels;
    std::set<std::string> names;

    for (const auto entry : kMenuEntries)
    {
        names.insert(labelFor(labels, entry));
    }

    EXPECT_EQ(kMenuEntries.size(), names.size());
    EXPECT_EQ(labels.playGame, labelFor(labels, MenuEntry::PlayGame));
    EXPECT_EQ(labels.loadReplay, labelFor(labels, MenuEntry::LoadReplay));
    EXPECT_EQ(labels.saveReplay, labelFor(labels, MenuEntry::SaveReplay));
    EXPECT_EQ(labels.resumeGame, labelFor(labels, MenuEntry::ResumeGame));
}

TEST(MenuLabelsTest, LabelFor_NamesEveryLanguageDifferently)
{
    const MenuLabels labels;
    std::set<std::string> names;

    for (const auto language : kMenuLanguages)
    {
        names.insert(labelFor(labels, language));
    }

    EXPECT_EQ(kMenuLanguages.size(), names.size());
    EXPECT_EQ(labels.english, labelFor(labels, MenuLanguage::English));
    EXPECT_EQ(labels.swedish, labelFor(labels, MenuLanguage::Swedish));
}
