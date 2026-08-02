#include "antwika/music_editor/EditorState.hpp"

#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <antwika/ui/ScrollChange.hpp>
#include <antwika/ui/TextEdit.hpp>
#include <antwika/ui/TextFieldSpec.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/music_editor/Score.hpp"

using antwika::music_editor::applyEdit;
using antwika::music_editor::applyScroll;
using antwika::music_editor::EditorState;
using antwika::music_editor::kCodeField;
using antwika::music_editor::KeyLayout;
using antwika::music_editor::kPanicButton;
using antwika::music_editor::kPlayButton;
using antwika::music_editor::openingSource;
using antwika::music_editor::openingState;
using antwika::ui::kCaretAtEnd;
using antwika::ui::kNoWidget;
using antwika::ui::ScrollChange;
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

    auto selected = state;
    selected.anchor = 0;
    EXPECT_NE(state, selected);

    auto scrolled = state;
    scrolled.scroll = 3;
    EXPECT_NE(state, scrolled);

    auto copied = state;
    copied.clipboard = "0 3";
    EXPECT_NE(state, copied);

    auto other = state;
    other.layout = KeyLayout::English;
    EXPECT_NE(state, other);

    auto open = state;
    open.layoutOpen = true;
    EXPECT_NE(state, open);

    auto speed = state;
    speed.speed = 3;
    EXPECT_NE(state, speed);

    auto speedOpen = state;
    speedOpen.speedOpen = true;
    EXPECT_NE(state, speedOpen);

    auto dragging = state;
    dragging.dragging = antwika::ui::DragHome::Text;
    EXPECT_NE(state, dragging);

    auto menued = state;
    menued.menuOpen = true;
    EXPECT_NE(state, menued);

    auto boxed = state;
    boxed.modal = antwika::music_editor::Modal::Save;
    EXPECT_NE(state, boxed);

    auto named = state;
    named.fileName = "beat";
    EXPECT_NE(state, named);

    auto caretted = state;
    caretted.fileCursor = 0;
    EXPECT_NE(state, caretted);

    auto noticed = state;
    noticed.notice = "name it first";
    EXPECT_NE(state, noticed);

    auto listed = state;
    listed.scores = {"beat"};
    EXPECT_NE(state, listed);
}

// Sorted, once each, however the saves arrive.
TEST(EditorStateTest, AddsAScoreInOrderAndOnlyOnce)
{
    auto state = openingState();

    antwika::music_editor::addScore(state, "middle");
    antwika::music_editor::addScore(state, "zed");
    antwika::music_editor::addScore(state, "alpha");
    antwika::music_editor::addScore(state, "middle");

    const std::vector<std::string> expected{"alpha", "middle", "zed"};

    EXPECT_EQ(state.scores, expected);
}

// The board this is written on, and nothing selected.
TEST(EditorStateTest, OpensOnTheSwedishBoardWithNothingSelected)
{
    const auto state = openingState();

    EXPECT_EQ(state.layout, KeyLayout::Swedish);
    EXPECT_FALSE(state.layoutOpen);
    EXPECT_FALSE(state.anchor.has_value());
    EXPECT_EQ(state.scroll, 0U);
    EXPECT_TRUE(state.clipboard.empty());
    EXPECT_EQ(state.dragging, antwika::ui::DragHome::None);
}

// A copy carries what was selected.
// And both ends of the selection come back with it.
TEST(EditorStateTest, AnEditCarriesTheSelectionAndWhatWasCopied)
{
    EditorState state;

    applyEdit(
        state,
        TextEdit{
            .field = kCodeField,
            .text = "0 3",
            .cursor = 1,
            .anchor = 3,
            .copied = "0 3"});

    EXPECT_EQ(state.cursor, 1U);
    EXPECT_EQ(state.anchor, 3U);
    EXPECT_EQ(state.clipboard, "0 3");

    // And a later edit copying nothing leaves the clipboard alone.
    applyEdit(
        state,
        TextEdit{.field = kCodeField, .text = "0 3", .cursor = 2});

    EXPECT_EQ(state.clipboard, "0 3");
}

TEST(EditorStateTest, AScrollReportMovesThePane)
{
    EditorState state;

    applyScroll(state, ScrollChange{.area = kCodeField, .line = 4});

    EXPECT_EQ(state.scroll, 4U);
}

// Which is what makes it safe to hand every frame's report straight in.
TEST(EditorStateTest, AScrollReportNamingAnythingElseIsIgnored)
{
    auto state = openingState();
    const auto before = state;

    applyScroll(state, ScrollChange{.area = kPlayButton, .line = 4});

    EXPECT_EQ(state, before);
}
