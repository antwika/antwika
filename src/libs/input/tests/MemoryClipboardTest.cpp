#include <gtest/gtest.h>

#include <antwika/input/MemoryClipboard.hpp>

using antwika::input::MemoryClipboard;

TEST(MemoryClipboardTest, Text_IsEmptyOnAFreshClipboard)
{
    const MemoryClipboard clipboard;

    EXPECT_EQ(clipboard.text(), "");
}

TEST(MemoryClipboardTest, SetText_HoldsWhatWasLastSet)
{
    MemoryClipboard clipboard;

    clipboard.setText("first");
    clipboard.setText("second");

    EXPECT_EQ(clipboard.text(), "second");
}

TEST(MemoryClipboardTest, SetText_EmptiesItWhenGivenNothing)
{
    MemoryClipboard clipboard;

    clipboard.setText("held");
    clipboard.setText("");

    EXPECT_EQ(clipboard.text(), "");
}
