#include <gtest/gtest.h>

#include "antwika/game/KeyboardLayout.hpp"
#include "antwika/game/MessageId.hpp"

using antwika::game::KeyboardLayout;
using antwika::game::keyboardLayoutFromName;
using antwika::game::keyboardLayoutLabel;
using antwika::game::keyboardLayoutName;
using antwika::game::kKeyboardLayouts;
using antwika::game::MessageId;

TEST(KeyboardLayoutTest, KeyboardLayoutFromName_ReadsEveryNameBack)
{
    for (const auto layout : kKeyboardLayouts)
    {
        EXPECT_EQ(
            keyboardLayoutFromName(keyboardLayoutName(layout)), layout);
    }
}

TEST(KeyboardLayoutTest, KeyboardLayoutName_AreThePersistedNames)
{
    EXPECT_EQ(
        keyboardLayoutName(KeyboardLayout::English), "english");
    EXPECT_EQ(
        keyboardLayoutName(KeyboardLayout::Swedish), "swedish");
}

TEST(KeyboardLayoutTest, KeyboardLayoutFromName_RefusesAnUnknownName)
{
    EXPECT_FALSE(keyboardLayoutFromName("dvorak").has_value());
}

TEST(KeyboardLayoutTest, KeyboardLayoutLabel_IsUniquePerLayout)
{
    EXPECT_EQ(
        keyboardLayoutLabel(KeyboardLayout::English),
        MessageId::KeyboardEnglish);
    EXPECT_EQ(
        keyboardLayoutLabel(KeyboardLayout::Swedish),
        MessageId::KeyboardSwedish);
}

TEST(KeyboardLayoutTest, KeyboardLayoutName_DefaultsToSwedish)
{
    EXPECT_EQ(
        antwika::game::kDefaultKeyboardLayout,
        KeyboardLayout::Swedish);
}
