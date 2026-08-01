#include <gtest/gtest.h>

#include "antwika/companion/Lineage.hpp"
#include "antwika/companion/SaveFormatError.hpp"

using antwika::companion::Lineage;
using antwika::companion::LineageMemory;
using antwika::companion::SaveFormatError;

namespace
{
    TEST(LineageTest, ANewFileIsOnItsFirstCompanionWithNoRecord)
    {
        const Lineage lineage;

        EXPECT_EQ(lineage.generation(), 1U);
        EXPECT_EQ(lineage.bestTicks(), 0U);
    }

    TEST(LineageTest, Record_KeepsOnlyTheLongestLife)
    {
        Lineage lineage;

        lineage.record(40);
        EXPECT_EQ(lineage.bestTicks(), 40U);

        lineage.record(10);
        EXPECT_EQ(lineage.bestTicks(), 40U);

        lineage.record(90);
        EXPECT_EQ(lineage.bestTicks(), 90U);
    }

    // The epilogue and a revival may both offer one companion's age.
    // So offering it twice has to be the same as offering it once.
    TEST(LineageTest, Record_IsIdempotentForOneCompanion)
    {
        Lineage lineage;

        lineage.record(40);
        lineage.record(40);

        EXPECT_EQ(lineage.bestTicks(), 40U);
    }

    TEST(LineageTest, Advance_MovesOnToTheNextCompanion)
    {
        Lineage lineage;

        lineage.advance();
        lineage.advance();

        EXPECT_EQ(lineage.generation(), 3U);
    }

    TEST(LineageTest, Remember_RoundTripsThroughTheConstructor)
    {
        Lineage lineage;
        lineage.record(120);
        lineage.advance();

        const LineageMemory memory = lineage.remember();
        const Lineage resumed(memory);

        EXPECT_EQ(resumed.remember(), memory);
        EXPECT_EQ(resumed.generation(), 2U);
        EXPECT_EQ(resumed.bestTicks(), 120U);
    }

    // Counted from one, so nothing is the zeroth of anything.
    TEST(LineageTest, Construction_RefusesAZerothCompanion)
    {
        EXPECT_THROW(
            (void)Lineage(LineageMemory{.generation = 0}),
            SaveFormatError);
    }
} // namespace
