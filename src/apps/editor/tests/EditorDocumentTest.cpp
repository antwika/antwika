#include <gtest/gtest.h>

#include <string>

#include "antwika/editor/editor/EditorDocument.hpp"

using antwika::editor::EditorDocument;
using antwika::editor::Snapshot;

namespace
{

    [[nodiscard]] Snapshot getSnapshotLit(const bool hiding)
    {
        Snapshot stepSnapshot{};
        stepSnapshot.map.settings.lighting = hiding;

        return stepSnapshot;
    }

}

TEST(EditorDocumentTest, StartFrom_KeepsThePathItWasStartedAt)
{
    EditorDocument document;

    document.startFrom("assets/maps/map.json");

    EXPECT_EQ(document.getPath(), "assets/maps/map.json");
    EXPECT_EQ(document.getStartPath(), "assets/maps/map.json");
}

TEST(EditorDocumentTest, OpenAt_LeavesTheStartingPathWhereItWas)
{
    EditorDocument document;

    document.startFrom("assets/maps/first.json");
    document.openAt("assets/maps/second.json");

    EXPECT_EQ(document.getPath(), "assets/maps/second.json");
    EXPECT_EQ(document.getStartPath(), "assets/maps/first.json");
}

TEST(EditorDocumentTest, GetSiblingPath_NamesAFileBesideTheOpenMap)
{
    EditorDocument document;

    document.startFrom("assets/maps/first.json");
    document.openAt("assets/towns/second.json");

    EXPECT_EQ(
        document.getSiblingPath("exit.json"), "assets/towns/exit.json");
}

TEST(EditorDocumentTest, GetStartSiblingPath_NamesAFileBesideTheStartingMap)
{
    EditorDocument document;

    document.startFrom("assets/maps/first.json");
    document.openAt("assets/towns/second.json");

    EXPECT_EQ(
        document.getStartSiblingPath("progress.json"),
        "assets/maps/progress.json");
}

TEST(EditorDocumentTest, IsDirty_ReadsFalseOnADocumentNobodyHasTouched)
{
    const EditorDocument document;

    EXPECT_FALSE(document.isDirty());
}

TEST(EditorDocumentTest, Push_LeavesTheDocumentDirty)
{
    EditorDocument document;

    document.push(getSnapshotLit(false));

    EXPECT_TRUE(document.isDirty());
}

TEST(EditorDocumentTest, MarkSaved_LeavesTheDocumentClean)
{
    EditorDocument document;

    document.markDirty();
    document.markSaved();

    EXPECT_FALSE(document.isDirty());
}

TEST(EditorDocumentTest, Undo_GivesBackTheStepThatWasPushed)
{
    EditorDocument document;

    const auto keptSnapshot = getSnapshotLit(true);
    document.push(keptSnapshot);

    const auto stepSnapshot = document.undo(getSnapshotLit(false));

    ASSERT_TRUE(stepSnapshot.has_value());
    EXPECT_TRUE(stepSnapshot->map.settings.lighting);
}

TEST(EditorDocumentTest, Undo_GivesNothingBackWithNoStepToTake)
{
    EditorDocument document;

    EXPECT_FALSE(document.undo(getSnapshotLit(false)).has_value());
}

TEST(EditorDocumentTest, Redo_GivesBackWhatAnUndoStepAwayFrom)
{
    EditorDocument document;

    const auto keptSnapshot = getSnapshotLit(false);
    document.push(keptSnapshot);

    (void)document.undo(getSnapshotLit(true));

    const auto stepSnapshot = document.redo(keptSnapshot);

    ASSERT_TRUE(stepSnapshot.has_value());
    EXPECT_TRUE(stepSnapshot->map.settings.lighting);
}

TEST(EditorDocumentTest, ForgetHistory_LeavesNothingToUndo)
{
    EditorDocument document;

    document.push(getSnapshotLit(false));
    document.forgetHistory();

    EXPECT_FALSE(document.undo(getSnapshotLit(false)).has_value());
}
