#include "antwika/music_editor/EditorState.hpp"

#include <cstddef>

#include <gtest/gtest.h>

#include <antwika/ui/TextEdit.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/music_editor/TrackPreset.hpp"

using antwika::music_editor::applyEdit;
using antwika::music_editor::EditorState;
using antwika::music_editor::fieldFor;
using antwika::music_editor::focusNext;
using antwika::music_editor::focusPrevious;
using antwika::music_editor::focusWidget;
using antwika::music_editor::kPlayButton;
using antwika::music_editor::kTrackCount;
using antwika::music_editor::openingState;
using antwika::ui::kNoWidget;
using antwika::ui::TextEdit;

// An editor that opened silent would give a newcomer nothing to change.
TEST(EditorStateTest, OpensWithSomethingAlreadyPlaying)
{
    const auto state = openingState();

    EXPECT_FALSE(state.paused);
    EXPECT_EQ(state.focused, 0U);

    for (std::size_t track = 0; track < kTrackCount; ++track)
    {
        EXPECT_FALSE(state.lines[track].empty()) << track;
    }
}

TEST(EditorStateTest, EveryFieldHasItsOwnIdAndNoneIsNoWidget)
{
    for (std::size_t track = 0; track < kTrackCount; ++track)
    {
        EXPECT_NE(fieldFor(track), kNoWidget);

        for (std::size_t other = 0; other < track; ++other)
        {
            EXPECT_NE(fieldFor(track), fieldFor(other));
        }
    }
}

TEST(EditorStateTest, AnEditGoesIntoTheLineItNames)
{
    EditorState state;

    applyEdit(
        state,
        TextEdit{.field = fieldFor(2), .text = "0 3", .cursor = 3});

    EXPECT_EQ(state.lines[2], "0 3");
    EXPECT_EQ(state.cursor, 3U);
    EXPECT_TRUE(state.lines[0].empty());
}

// Which is what makes it safe to hand every frame's edit straight in.
TEST(EditorStateTest, AnEditNamingNoLineOfThisEditorIsIgnored)
{
    EditorState state;
    const auto before = state;

    applyEdit(
        state, TextEdit{.field = kPlayButton, .text = "0 3"});

    EXPECT_EQ(state, before);
}

TEST(EditorStateTest, FocusWrapsBothWays)
{
    EditorState state;

    focusPrevious(state);
    EXPECT_EQ(state.focused, kTrackCount - 1);

    focusNext(state);
    EXPECT_EQ(state.focused, 0U);

    for (std::size_t step = 0; step < kTrackCount; ++step)
    {
        focusNext(state);
    }

    EXPECT_EQ(state.focused, 0U);
}

TEST(EditorStateTest, MovingTheFocusPutsTheCaretAtTheEnd)
{
    EditorState state;
    state.cursor = 0;

    focusNext(state);

    EXPECT_EQ(state.cursor, antwika::ui::kCaretAtEnd);
}

TEST(EditorStateTest, AWidgetThatIsAFieldTakesTheFocus)
{
    EditorState state;

    EXPECT_TRUE(focusWidget(state, fieldFor(3)));
    EXPECT_EQ(state.focused, 3U);

    EXPECT_FALSE(focusWidget(state, kPlayButton));
    EXPECT_EQ(state.focused, 3U);

    EXPECT_FALSE(focusWidget(state, kNoWidget));
    EXPECT_EQ(state.focused, 3U);
}

TEST(EditorStateTest, ComparesFieldByField)
{
    const auto state = openingState();

    EXPECT_EQ(state, state);
    EXPECT_NE(state, EditorState{});

    auto moved = state;
    moved.focused = 1;
    EXPECT_NE(state, moved);

    auto held = state;
    held.paused = true;
    EXPECT_NE(state, held);

    auto typed = state;
    typed.cursor = 0;
    EXPECT_NE(state, typed);
}
