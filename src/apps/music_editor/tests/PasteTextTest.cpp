#include "antwika/music_editor/PasteText.hpp"

#include <string>

#include <gtest/gtest.h>

using antwika::music_editor::kMaxPasteBytes;
using antwika::music_editor::pasteableTextOf;

TEST(PasteTextTest, KeepsWhatTheGrammarAndThePaneCanShow)
{
    const std::string score{"$: drum.n(\"0(3,8)\").gain(.2)\n\tend"};

    EXPECT_EQ(pasteableTextOf(score), score);
}

TEST(PasteTextTest, ReadsCrlfAndALoneCrAsOneNewlineEach)
{
    EXPECT_EQ(pasteableTextOf("a\r\nb\rc"), "a\nb\nc");

    // A CR ending the text has no LF to pair with.
    EXPECT_EQ(pasteableTextOf("a\r"), "a\n");
}

// A Latin-1 clipboard's bytes are not valid UTF-8.
// Unfiltered, they used to end a --record run mid-dispatch.
TEST(PasteTextTest, DropsBytesThePaneCannotShow)
{
    EXPECT_EQ(pasteableTextOf("caf\xE9 au lait"), "caf au lait");

    // Control bytes enter no document either.
    EXPECT_EQ(pasteableTextOf(std::string{"a\x07\x01z"}), "az");
}

TEST(PasteTextTest, StopsAtTheCap)
{
    const std::string flood(kMaxPasteBytes + 100, 'x');

    EXPECT_EQ(pasteableTextOf(flood).size(), kMaxPasteBytes);
}

TEST(PasteTextTest, AnUnpasteableClipboardYieldsNothing)
{
    EXPECT_EQ(pasteableTextOf("\x80\x9F"), "");
}
