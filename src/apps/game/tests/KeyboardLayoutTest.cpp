#include <gtest/gtest.h>

#include "antwika/game/KeyboardLayout.hpp"
#include "antwika/game/MessageId.hpp"

using antwika::game::KeyboardLayout;
using antwika::game::keyboardLayoutFromName;
using antwika::game::keyboardLayoutLabel;
using antwika::game::keyboardLayoutName;
using antwika::game::kKeyboardLayouts;
using antwika::game::MessageId;

TEST(KeyboardLayoutTest, EveryLayoutsNameReadsBack)
{
    for (const auto layout : kKeyboardLayouts)
    {
        EXPECT_EQ(
            keyboardLayoutFromName(keyboardLayoutName(layout)), layout);
    }
}

TEST(KeyboardLayoutTest, TheNamesAreThePersistedOnes)
{
    // Part of two formats, so they may not change once written.
    EXPECT_EQ(
        keyboardLayoutName(KeyboardLayout::English), "english");
    EXPECT_EQ(
        keyboardLayoutName(KeyboardLayout::Swedish), "swedish");
}

TEST(KeyboardLayoutTest, ANameNoLayoutGoesByAnswersNothing)
{
    EXPECT_FALSE(keyboardLayoutFromName("dvorak").has_value());
}

TEST(KeyboardLayoutTest, EveryLayoutHasItsOwnCaption)
{
    EXPECT_EQ(
        keyboardLayoutLabel(KeyboardLayout::English),
        MessageId::KeyboardEnglish);
    EXPECT_EQ(
        keyboardLayoutLabel(KeyboardLayout::Swedish),
        MessageId::KeyboardSwedish);
}

TEST(KeyboardLayoutTest, TheShippedDefaultIsTheSwedishBoard)
{
    // The board this project is written on -- see the header.
    EXPECT_EQ(
        antwika::game::kDefaultKeyboardLayout,
        KeyboardLayout::Swedish);
}
