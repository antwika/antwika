#include <gtest/gtest.h>

#include <cstdint>

#include "antwika/editor/editor/History.hpp"

namespace
{

    struct Step final
    {
        std::int32_t mark = 0;
    };

    using History = antwika::editor::History<Step>;

    using antwika::editor::kDefaultHistoryDepth;

    [[nodiscard]] Step stepOf(const std::int32_t mark)
    {
        return Step{.mark = mark};
    }

    TEST(HistoryTest, Undo_GivesNothingWithNothingKept)
    {
        History history;

        EXPECT_FALSE(history.undo(stepOf(1)).has_value());
        EXPECT_EQ(history.getUndoCount(), 0U);
    }

    TEST(HistoryTest, Undo_GivesBackTheSnapshotKeptBefore)
    {
        History history;

        history.push(stepOf(1));

        const auto undone = history.undo(stepOf(2));

        ASSERT_TRUE(undone.has_value());
        EXPECT_EQ(undone->mark, 1);
        EXPECT_EQ(history.getUndoCount(), 0U);
        EXPECT_EQ(history.getRedoCount(), 1U);
    }

    TEST(HistoryTest, Redo_GivesBackWhatUndoingLeft)
    {
        History history;

        history.push(stepOf(1));

        const auto undone = history.undo(stepOf(2));

        ASSERT_TRUE(undone.has_value());

        const auto redone = history.redo(*undone);

        ASSERT_TRUE(redone.has_value());
        EXPECT_EQ(redone->mark, 2);
        EXPECT_EQ(history.getUndoCount(), 1U);
        EXPECT_EQ(history.getRedoCount(), 0U);
    }

    TEST(HistoryTest, Redo_GivesNothingWithNothingUndone)
    {
        History history;

        history.push(stepOf(1));

        EXPECT_FALSE(history.redo(stepOf(2)).has_value());
    }

    TEST(HistoryTest, Push_LetsGoOfWhatWasUndone)
    {
        History history;

        history.push(stepOf(1));

        const auto undone = history.undo(stepOf(2));

        ASSERT_TRUE(undone.has_value());

        history.push(stepOf(3));

        EXPECT_EQ(history.getRedoCount(), 0U);
        EXPECT_FALSE(history.redo(stepOf(4)).has_value());
    }

    TEST(HistoryTest, Push_HoldsNoMoreThanTheDepth)
    {
        History history;

        for (std::int32_t mark = 0;
             mark
             < static_cast<std::int32_t>(kDefaultHistoryDepth) + 8;
             ++mark)
        {
            history.push(stepOf(mark));
        }

        EXPECT_EQ(history.getUndoCount(), kDefaultHistoryDepth);
    }

    TEST(HistoryTest, Push_HoldsToADepthAskedForInstead)
    {
        antwika::editor::History<Step, 2> history;

        for (std::int32_t mark = 0; mark < 5; ++mark)
        {
            history.push(stepOf(mark));
        }

        EXPECT_EQ(history.getUndoCount(), 2U);
    }

    TEST(HistoryTest, Undo_WalksBackStepByStep)
    {
        History history;

        history.push(stepOf(1));
        history.push(stepOf(2));

        auto step = stepOf(3);

        for (const std::int32_t mark : {2, 1})
        {
            const auto undone = history.undo(step);

            ASSERT_TRUE(undone.has_value());
            EXPECT_EQ(undone->mark, mark);

            step = *undone;
        }

        EXPECT_FALSE(history.undo(step).has_value());
    }

    TEST(HistoryTest, Clear_LetsGoOfEverySnapshot)
    {
        History history;

        history.push(stepOf(1));

        const auto undone = history.undo(stepOf(2));

        ASSERT_TRUE(undone.has_value());

        history.clear();

        EXPECT_EQ(history.getUndoCount(), 0U);
        EXPECT_EQ(history.getRedoCount(), 0U);
    }

}
