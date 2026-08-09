#include <gtest/gtest.h>

#include <antwika/notation/NotationError.hpp>
#include <antwika/pattern/ParamValue.hpp>

#include "antwika/music_editor/NoteWords.hpp"
#include "antwika/music_editor/TrackPreset.hpp"

using antwika::music_editor::kNote;
using antwika::music_editor::kSpanBegin;
using antwika::music_editor::kSpanLength;
using antwika::music_editor::NoteWords;
using antwika::pattern::ParamValue;

TEST(NoteWordsTest, Read_CarriesThePitchAndItsSpan)
{
    const NoteWords words;

    const auto controls = words.read("12", 7);

    EXPECT_EQ(controls.get(kNote), ParamValue(12));
    EXPECT_EQ(controls.get(kSpanBegin), ParamValue(7));
    EXPECT_EQ(controls.get(kSpanLength), ParamValue(2));
}

TEST(NoteWordsTest, Read_RefusesWhatIsNotANumber)
{
    const NoteWords words;

    EXPECT_THROW(
        (void)words.read("bd", 0), antwika::notation::NotationError);
}
