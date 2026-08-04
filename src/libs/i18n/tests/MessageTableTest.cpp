#include <cstdint>

#include <gtest/gtest.h>

#include "antwika/i18n/MessageTable.hpp"

namespace antwika::i18n
{

    namespace
    {

        enum class TableId : std::uint16_t
        {
            First,
            Second,
            Count,
        };

        // Each table below leaves out exactly one thing.
        // That is the shape a forgotten line in a Messages.cpp takes.
        // The arrays are sized by the enumeration's own Count.
        // So a line nobody wrote is a value-initialised entry.
        constexpr MessageTable<TableId> kComplete{
            .names{{
                {TableId::First, "First"},
                {TableId::Second, "Second"},
            }},
            .english{{
                {TableId::First, "one"},
                {TableId::Second, "two"},
            }},
            .swedish{{
                {TableId::First, "ett"},
                {TableId::Second, "tva"},
            }},
        };

        constexpr MessageTable<TableId> kNameMissing{
            .names{{
                {TableId::First, "First"},
            }},
            .english{{
                {TableId::First, "one"},
                {TableId::Second, "two"},
            }},
            .swedish{{
                {TableId::First, "ett"},
                {TableId::Second, "tva"},
            }},
        };

        constexpr MessageTable<TableId> kEnglishMissing{
            .names{{
                {TableId::First, "First"},
                {TableId::Second, "Second"},
            }},
            .english{{
                {TableId::First, "one"},
            }},
            .swedish{{
                {TableId::First, "ett"},
                {TableId::Second, "tva"},
            }},
        };

        constexpr MessageTable<TableId> kEnglishEmpty{
            .names{{
                {TableId::First, "First"},
                {TableId::Second, "Second"},
            }},
            .english{{
                {TableId::First, "one"},
                {TableId::Second, ""},
            }},
            .swedish{{
                {TableId::First, "ett"},
                {TableId::Second, "tva"},
            }},
        };

        constexpr MessageTable<TableId> kSwedishMissing{
            .names{{
                {TableId::First, "First"},
                {TableId::Second, "Second"},
            }},
            .english{{
                {TableId::First, "one"},
                {TableId::Second, "two"},
            }},
            .swedish{{
                {TableId::First, "ett"},
            }},
        };

        constexpr MessageTable<TableId> kSwedishEmpty{
            .names{{
                {TableId::First, "First"},
                {TableId::Second, "Second"},
            }},
            .english{{
                {TableId::First, "one"},
                {TableId::Second, "two"},
            }},
            .swedish{{
                {TableId::First, "ett"},
                {TableId::Second, ""},
            }},
        };

    } // namespace

    TEST(MessageTableTest, MessageCount_ReadsTheTrailingEnumerator)
    {
        EXPECT_EQ(messageCount<TableId>, 2U);
        EXPECT_EQ(kComplete.names.size(), messageCount<TableId>);
        EXPECT_EQ(kComplete.english.size(), messageCount<TableId>);
        EXPECT_EQ(kComplete.swedish.size(), messageCount<TableId>);
    }

    TEST(MessageTableTest, IsComplete_AcceptsATableWithNothingMissing)
    {
        EXPECT_TRUE(isComplete(kComplete));
    }

    // The static_assert a module writes beside its table is this call.
    // Reading it at run time too is what lets a test watch it refuse.
    TEST(MessageTableTest, IsComplete_AnswersAtCompileTimeToo)
    {
        static_assert(isComplete(kComplete));
        static_assert(!isComplete(kSwedishMissing));

        SUCCEED();
    }

    TEST(MessageTableTest, IsComplete_RefusesAnIdThatWasNeverNamed)
    {
        EXPECT_FALSE(isComplete(kNameMissing));
    }

    TEST(MessageTableTest, IsComplete_RefusesAMissingEnglishEntry)
    {
        EXPECT_FALSE(isComplete(kEnglishMissing));
    }

    TEST(MessageTableTest, IsComplete_RefusesAnEmptyEnglishEntry)
    {
        EXPECT_FALSE(isComplete(kEnglishEmpty));
    }

    TEST(MessageTableTest, IsComplete_RefusesAMissingSwedishEntry)
    {
        EXPECT_FALSE(isComplete(kSwedishMissing));
    }

    TEST(MessageTableTest, IsComplete_RefusesAnEmptySwedishEntry)
    {
        EXPECT_FALSE(isComplete(kSwedishEmpty));
    }

} // namespace antwika::i18n
