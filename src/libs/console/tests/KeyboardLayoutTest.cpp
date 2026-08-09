#include <gtest/gtest.h>

#include "antwika/console/KeyboardLayout.hpp"

using antwika::console::keyboardLayoutFromName;
using antwika::console::KeyboardLayout;
using antwika::console::keyboardLayoutName;
using antwika::console::kKeyboardLayoutCount;
using antwika::console::kKeyboardLayouts;

TEST(KeyboardLayoutTest, KKeyboardLayouts_ListsEveryLayoutOnce)
{
    ASSERT_EQ(kKeyboardLayoutCount, 2U);
    EXPECT_EQ(kKeyboardLayouts[0], KeyboardLayout::English);
    EXPECT_EQ(kKeyboardLayouts[1], KeyboardLayout::Swedish);
}

TEST(KeyboardLayoutTest, KeyboardLayoutName_SpellsEveryLayout)
{
    EXPECT_EQ(keyboardLayoutName(KeyboardLayout::English), "english");
    EXPECT_EQ(keyboardLayoutName(KeyboardLayout::Swedish), "swedish");
}

TEST(KeyboardLayoutTest, KeyboardLayoutFromName_ReadsBackEveryName)
{
    EXPECT_EQ(keyboardLayoutFromName("english"), KeyboardLayout::English);
    EXPECT_EQ(keyboardLayoutFromName("swedish"), KeyboardLayout::Swedish);
}

TEST(KeyboardLayoutTest, KeyboardLayoutFromName_HasNoLayoutForAnythingElse)
{
    EXPECT_FALSE(keyboardLayoutFromName("").has_value());
    EXPECT_FALSE(keyboardLayoutFromName("English").has_value());
    EXPECT_FALSE(keyboardLayoutFromName("dvorak").has_value());
}
