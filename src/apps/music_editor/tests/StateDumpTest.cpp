#include "antwika/music_editor/StateDump.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>

#include <gtest/gtest.h>

#include <antwika/pattern/PatternError.hpp>
#include <antwika/sequencer/Rational.hpp>
#include <antwika/sequencer/TempoMap.hpp>
#include <antwika/ui/TextAreaSpec.hpp>

#include "antwika/music_editor/EditorState.hpp"
#include "antwika/music_editor/StateDumpError.hpp"

using antwika::music_editor::EditorDump;
using antwika::music_editor::editorDumpFromJson;
using antwika::music_editor::editorDumpToJson;
using antwika::music_editor::EditorState;
using antwika::music_editor::KeyLayout;
using antwika::music_editor::Modal;
using antwika::music_editor::openingState;
using antwika::music_editor::PlaybackMemory;
using antwika::music_editor::StateDumpError;
using antwika::sequencer::Rational;
using antwika::sequencer::TempoMap;

namespace
{
    // A session well away from every default, one field at a time.
    [[nodiscard]] EditorDump busyDump()
    {
        EditorDump dump;

        dump.editor = openingState();
        dump.editor.source = "$: drum.n(\"0*4\")\n";
        dump.editor.cursor = 5;
        dump.editor.anchor = 2;
        dump.editor.scroll = 3;
        dump.editor.clipboard = "n(\"0\")";
        dump.editor.layout = KeyLayout::English;
        dump.editor.layoutOpen = true;
        dump.editor.dragging = antwika::ui::DragHome::Track;
        dump.editor.paused = true;
        dump.editor.menuOpen = true;
        dump.editor.speed = 3;
        dump.editor.speedOpen = true;
        dump.editor.modal = Modal::Save;
        dump.editor.fileName = "my-song";
        dump.editor.fileCursor = 4;
        dump.editor.notice = "nothing to save";
        dump.editor.scores = {"anthem", "lullaby"};

        TempoMap tempo(Rational(48000));
        tempo.addSegment(Rational(2), Rational(24000));

        dump.playback = PlaybackMemory{
            .segments = tempo.segments(),
            .retimed = Rational(2),
            .played = 25,
            .counter = 40,
            .queued = 120000,
            .pausedFrames = 4800,
            .voiceCount = 2};

        return dump;
    }
} // namespace

TEST(StateDumpTest, RoundTripsABusySession)
{
    const auto dump = busyDump();

    EXPECT_EQ(editorDumpFromJson(editorDumpToJson(dump)), dump);
}

// The defaults hold two shapes worth their own trip.
// No anchor, and a caret standing at the very end sentinel.
TEST(StateDumpTest, RoundTripsTheOpeningStateAndItsAbsentAnchor)
{
    EditorDump dump;
    dump.editor = openingState();

    TempoMap tempo(Rational(48000));
    dump.playback.segments = tempo.segments();

    const auto decoded = editorDumpFromJson(editorDumpToJson(dump));

    EXPECT_EQ(decoded, dump);
    EXPECT_EQ(decoded.editor.anchor, std::nullopt);
    EXPECT_EQ(decoded.editor.cursor, antwika::ui::kCaretAtEnd);
}

// Every persisted enum name, both directions.
TEST(StateDumpTest, RoundTripsEveryPersistedName)
{
    auto dump = busyDump();

    for (const auto layout : {KeyLayout::Swedish, KeyLayout::English})
    {
        for (const auto modal : {Modal::None, Modal::Save, Modal::Load})
        {
            for (const auto dragging :
                 {antwika::ui::DragHome::None,
                  antwika::ui::DragHome::Text,
                  antwika::ui::DragHome::Track})
            {
                dump.editor.layout = layout;
                dump.editor.modal = modal;
                dump.editor.dragging = dragging;

                EXPECT_EQ(
                    editorDumpFromJson(editorDumpToJson(dump)), dump);
            }
        }
    }
}

TEST(StateDumpTest, RefusesAKeyboardItDoesNotKnow)
{
    auto document = editorDumpToJson(busyDump());
    document["editor"]["layout"] = "dvorak";

    EXPECT_THROW((void)editorDumpFromJson(document), StateDumpError);
}

TEST(StateDumpTest, RefusesAModalItDoesNotKnow)
{
    auto document = editorDumpToJson(busyDump());
    document["editor"]["modal"] = "quit";

    EXPECT_THROW((void)editorDumpFromJson(document), StateDumpError);
}

TEST(StateDumpTest, RefusesADragHomeItDoesNotKnow)
{
    auto document = editorDumpToJson(busyDump());
    document["editor"]["dragging"] = "menu";

    EXPECT_THROW((void)editorDumpFromJson(document), StateDumpError);
}

// An index into kSpeeds, so the table's size is the bound.
TEST(StateDumpTest, RefusesASpeedIndexPastTheTable)
{
    auto document = editorDumpToJson(busyDump());
    document["editor"]["speed"] =
        antwika::music_editor::kSpeeds.size();

    EXPECT_THROW((void)editorDumpFromJson(document), StateDumpError);
}

TEST(StateDumpTest, RefusesAFractionWithAZeroDenominator)
{
    auto document = editorDumpToJson(busyDump());
    document["playback"]["retimed"]["den"] = 0;

    EXPECT_THROW((void)editorDumpFromJson(document), StateDumpError);
}

TEST(StateDumpTest, RefusesADocumentMissingAMember)
{
    auto document = editorDumpToJson(busyDump());
    document["editor"].erase("source");

    EXPECT_THROW((void)editorDumpFromJson(document), StateDumpError);
}

TEST(StateDumpTest, RefusesAMemberNobodyDefined)
{
    auto document = editorDumpToJson(busyDump());
    document["editor"]["volume"] = 11;

    EXPECT_THROW((void)editorDumpFromJson(document), StateDumpError);
}

TEST(StateDumpTest, RefusesAnEmptyTempoTable)
{
    auto document = editorDumpToJson(busyDump());
    document["playback"]["segments"] = nlohmann::json::array();

    EXPECT_THROW((void)editorDumpFromJson(document), StateDumpError);
}

// The map's own zero is where every table begins.
TEST(StateDumpTest, RefusesAFirstSegmentAwayFromCycleZero)
{
    auto document = editorDumpToJson(busyDump());
    document["playback"]["segments"][0]["startCycle"]["num"] = 1;

    EXPECT_THROW((void)editorDumpFromJson(document), StateDumpError);
}

// A table the TempoMap itself refuses: two segments at one cycle.
TEST(StateDumpTest, RefusesATempoTableTheMapRefuses)
{
    auto document = editorDumpToJson(busyDump());
    document["playback"]["segments"][1]["startCycle"]["num"] = 0;

    EXPECT_THROW((void)editorDumpFromJson(document), StateDumpError);
}

// A tempo of no frames at all is the map's other refusal.
TEST(StateDumpTest, RefusesATempoThatWouldNeverAdvance)
{
    auto document = editorDumpToJson(busyDump());
    document["playback"]["segments"][0]["framesPerCycle"]["num"] = 0;

    EXPECT_THROW((void)editorDumpFromJson(document), StateDumpError);
}

// The defaulted comparison must consult every field, not stop early.
TEST(StateDumpTest, PlaybackMemoryEqualityComparesEveryField)
{
    const auto base = busyDump().playback;

    auto tabled = base;
    tabled.segments.pop_back();
    EXPECT_NE(base, tabled);

    auto landed = base;
    landed.retimed = Rational(3);
    EXPECT_NE(base, landed);

    auto ticked = base;
    ticked.played = 26;
    EXPECT_NE(base, ticked);

    auto counted = base;
    counted.counter = 41;
    EXPECT_NE(base, counted);

    auto fed = base;
    fed.queued = 120001;
    EXPECT_NE(base, fed);

    auto stood = base;
    stood.pausedFrames = 4801;
    EXPECT_NE(base, stood);

    auto grown = base;
    grown.voiceCount = 3;
    EXPECT_NE(base, grown);
}

// Both halves count, not only whichever one is compared first.
TEST(StateDumpTest, EditorDumpEqualityComparesBothHalves)
{
    const auto base = busyDump();

    auto typed = base;
    typed.editor.cursor = 6;
    EXPECT_NE(base, typed);

    auto clocked = base;
    clocked.playback.played = 26;
    EXPECT_NE(base, clocked);
}

// A table whose arithmetic will not fit is the pattern's refusal.
// The decode only rewraps the sequencer's own category.
TEST(StateDumpTest, AnOverflowingTempoTableEscapesAsThePatternsError)
{
    auto document = editorDumpToJson(busyDump());
    document["playback"]["segments"][1]["startCycle"]["num"] =
        std::numeric_limits<std::int64_t>::max();

    EXPECT_THROW(
        (void)editorDumpFromJson(document),
        antwika::pattern::PatternError);
}

// startFrame is never written; the decode recomputes every one.
TEST(StateDumpTest, RecomputesEverySegmentsStartFrameOnDecode)
{
    const auto dump = busyDump();
    const auto decoded = editorDumpFromJson(editorDumpToJson(dump));

    ASSERT_EQ(decoded.playback.segments.size(), 2U);
    EXPECT_EQ(decoded.playback.segments[1].startFrame, 96000U);
}
