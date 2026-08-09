#include <gtest/gtest.h>

#include <string>

#include "antwika/music_editor/PasteText.hpp"

using antwika::music_editor::kMaxPasteBytes;
using antwika::music_editor::pasteableTextOf;

TEST(PasteTextTest, PasteableTextOf_KeepsWhatTheGrammarAndThePaneCanShow)
{
    const std::string score{"$: drum.n(\"0(3,8)\").gain(.2)\n\tend"};

    EXPECT_EQ(pasteableTextOf(score), score);
}

TEST(PasteTextTest, PasteableTextOf_ReadsCrlfAndALoneCrAsOneNewlineEach)
{
    EXPECT_EQ(pasteableTextOf("a\r\nb\rc"), "a\nb\nc");

    EXPECT_EQ(pasteableTextOf("a\r"), "a\n");
}

TEST(PasteTextTest, PasteableTextOf_DropsBytesThePaneCannotShow)
{
    EXPECT_EQ(pasteableTextOf("caf\xE9 au lait"), "caf au lait");

    EXPECT_EQ(pasteableTextOf(std::string{"a\x07\x01z"}), "az");
}

TEST(PasteTextTest, PasteableTextOf_StopsAtTheCap)
{
    const std::string flood(kMaxPasteBytes + 100, 'x');

    EXPECT_EQ(pasteableTextOf(flood).size(), kMaxPasteBytes);
}

TEST(PasteTextTest, PasteableTextOf_AnUnpasteableClipboardYieldsNothing)
{
    EXPECT_EQ(pasteableTextOf("\x80\x9F"), "");
}
