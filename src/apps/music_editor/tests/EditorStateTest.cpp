#include "antwika/music_editor/EditorState.hpp"

#include <gtest/gtest.h>

#include <antwika/ui/TextEdit.hpp>
#include <antwika/ui/TextFieldSpec.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/music_editor/Score.hpp"

using antwika::music_editor::applyEdit;
using antwika::music_editor::EditorState;
using antwika::music_editor::kCodeField;
using antwika::music_editor::kPanicButton;
using antwika::music_editor::kPlayButton;
using antwika::music_editor::openingSource;
using antwika::music_editor::openingState;
using antwika::ui::kCaretAtEnd;
using antwika::ui::kNoWidget;
using antwika::ui::TextEdit;

// An editor that opened silent would give a newcomer nothing to change.
TEST(EditorStateTest, OpensWithSomethingAlreadyPlaying)
{
    const auto state = openingState();

    EXPECT_EQ(state.source, openingSource());
    EXPECT_FALSE(state.source.empty());
    EXPECT_EQ(state.cursor, kCaretAtEnd);
    EXPECT_FALSE(state.paused);
}

TEST(EditorStateTest, TheThreeWidgetIdsAreDistinctAndNamed)
{
    EXPECT_NE(kCodeField, kNoWidget);
    EXPECT_NE(kPlayButton, kNoWidget);
    EXPECT_NE(kPanicButton, kNoWidget);

    EXPECT_NE(kCodeField, kPlayButton);
    EXPECT_NE(kCodeField, kPanicButton);
    EXPECT_NE(kPlayButton, kPanicButton);
}

TEST(EditorStateTest, AnEditGoesIntoTheDocumentAndMovesTheCaret)
{
    EditorState state;

    applyEdit(
        state,
        TextEdit{
            .field = kCodeField,
            .text = "$: bass 0 3",
            .cursor = 4});

    EXPECT_EQ(state.source, "$: bass 0 3");
    EXPECT_EQ(state.cursor, 4U);
    EXPECT_FALSE(state.paused);
}

// Which is what makes it safe to hand every frame's edit straight in.
TEST(EditorStateTest, AnEditNamingAnythingElseIsIgnored)
{
    auto state = openingState();
    const auto before = state;

    applyEdit(
        state, TextEdit{.field = kPlayButton, .text = "gone", .cursor = 0});

    EXPECT_EQ(state, before);

    applyEdit(
        state, TextEdit{.field = kNoWidget, .text = "gone", .cursor = 0});

    EXPECT_EQ(state, before);
}

TEST(EditorStateTest, ComparesFieldByField)
{
    const auto state = openingState();

    EXPECT_EQ(state, state);
    EXPECT_NE(state, EditorState{});

    auto typed = state;
    typed.source += "\n";
    EXPECT_NE(state, typed);

    auto moved = state;
    moved.cursor = 0;
    EXPECT_NE(state, moved);

    auto held = state;
    held.paused = true;
    EXPECT_NE(state, held);
}
