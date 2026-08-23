#include <gtest/gtest.h>

#include <antwika/input/InMemoryClipboard.hpp>

using antwika::input::InMemoryClipboard;

TEST(InMemoryClipboardTest, Text_IsEmptyOnAFreshClipboard)
{
    const InMemoryClipboard clipboard;

    EXPECT_EQ(clipboard.getText(), "");
}

TEST(InMemoryClipboardTest, SetText_HoldsWhatWasLastSet)
{
    InMemoryClipboard clipboard;

    clipboard.setText("first");
    clipboard.setText("second");

    EXPECT_EQ(clipboard.getText(), "second");
}

TEST(InMemoryClipboardTest, SetText_EmptiesItWhenGivenNothing)
{
    InMemoryClipboard clipboard;

    clipboard.setText("held");
    clipboard.setText("");

    EXPECT_EQ(clipboard.getText(), "");
}
