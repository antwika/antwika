#include <gtest/gtest.h>

#include <cstdint>

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

    }

    TEST(MessageTableTest, MessageCount_ReadsTheTrailingEnumerator)
    {
        EXPECT_EQ(kMessageCount<TableId>, 2U);
        EXPECT_EQ(kComplete.names.size(), kMessageCount<TableId>);
        EXPECT_EQ(kComplete.english.size(), kMessageCount<TableId>);
        EXPECT_EQ(kComplete.swedish.size(), kMessageCount<TableId>);
    }

    TEST(MessageTableTest, IsComplete_AcceptsATableWithNothingMissing)
    {
        EXPECT_TRUE(isComplete(kComplete));
    }

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

}
