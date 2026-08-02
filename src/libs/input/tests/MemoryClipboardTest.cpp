#include <gtest/gtest.h>

#include <antwika/input/MemoryClipboard.hpp>

using antwika::input::MemoryClipboard;

TEST(MemoryClipboardTest, OpensHoldingNothing)
{
    const MemoryClipboard clipboard;

    EXPECT_EQ(clipboard.text(), "");
}

TEST(MemoryClipboardTest, HoldsWhatWasLastSet)
{
    MemoryClipboard clipboard;

    clipboard.setText("first");
    clipboard.setText("second");

    EXPECT_EQ(clipboard.text(), "second");
}

TEST(MemoryClipboardTest, SettingNothingEmptiesIt)
{
    MemoryClipboard clipboard;

    clipboard.setText("held");
    clipboard.setText("");

    EXPECT_EQ(clipboard.text(), "");
}
