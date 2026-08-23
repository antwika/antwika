#include <gtest/gtest.h>

#include <string>

#include "antwika/editor/editor/EditorDocument.hpp"

using antwika::editor::EditorDocument;
using antwika::map::Snapshot;

namespace
{

    [[nodiscard]] Snapshot snapshotHiding(const bool hiding)
    {
        Snapshot stepSnapshot{};
        stepSnapshot.map.settings.hideAboveLevel = hiding;

        return stepSnapshot;
    }

}

TEST(EditorDocumentTest, StartFrom_KeepsThePathItWasStartedAt)
{
    EditorDocument document;

    document.startFrom("assets/maps/map.json");

    EXPECT_EQ(document.path(), "assets/maps/map.json");
    EXPECT_EQ(document.startPath(), "assets/maps/map.json");
}

TEST(EditorDocumentTest, OpenAt_LeavesTheStartingPathWhereItWas)
{
    EditorDocument document;

    document.startFrom("assets/maps/first.json");
    document.openAt("assets/maps/second.json");

    EXPECT_EQ(document.path(), "assets/maps/second.json");
    EXPECT_EQ(document.startPath(), "assets/maps/first.json");
}

TEST(EditorDocumentTest, IsDirty_ReadsFalseOnADocumentNobodyHasTouched)
{
    const EditorDocument document;

    EXPECT_FALSE(document.isDirty());
}

TEST(EditorDocumentTest, Push_LeavesTheDocumentDirty)
{
    EditorDocument document;

    document.push(snapshotHiding(false));

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

    const auto keptSnapshot = snapshotHiding(true);
    document.push(keptSnapshot);

    const auto stepSnapshot = document.undo(snapshotHiding(false));

    ASSERT_TRUE(stepSnapshot.has_value());
    EXPECT_TRUE(stepSnapshot->map.settings.hideAboveLevel);
}

TEST(EditorDocumentTest, Undo_GivesNothingBackWithNoStepToTake)
{
    EditorDocument document;

    EXPECT_FALSE(document.undo(snapshotHiding(false)).has_value());
}

TEST(EditorDocumentTest, Redo_GivesBackWhatAnUndoStepAwayFrom)
{
    EditorDocument document;

    const auto keptSnapshot = snapshotHiding(false);
    document.push(keptSnapshot);

    (void)document.undo(snapshotHiding(true));

    const auto stepSnapshot = document.redo(keptSnapshot);

    ASSERT_TRUE(stepSnapshot.has_value());
    EXPECT_TRUE(stepSnapshot->map.settings.hideAboveLevel);
}

TEST(EditorDocumentTest, ForgetHistory_LeavesNothingToUndo)
{
    EditorDocument document;

    document.push(snapshotHiding(false));
    document.forgetHistory();

    EXPECT_FALSE(document.undo(snapshotHiding(false)).has_value());
}
