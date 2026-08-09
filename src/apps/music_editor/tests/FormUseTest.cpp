#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

#include <antwika/pattern/Cycle.hpp>
#include <antwika/pattern/Span.hpp>

#include "antwika/music_editor/FormUse.hpp"
#include "antwika/music_editor/ScoreError.hpp"

using antwika::music_editor::FormUse;
using antwika::music_editor::periodOf;
using antwika::music_editor::readBarsLine;
using antwika::music_editor::readFormLine;
using antwika::music_editor::readPartLine;
using antwika::music_editor::resolveBars;
using antwika::music_editor::ScoreError;
using antwika::music_editor::windowsFor;
using antwika::pattern::Cycle;
using antwika::pattern::Span;

namespace
{
    [[nodiscard]] Span cycles(
        const std::int64_t from, const std::int64_t to)
    {
        return Span(Cycle(from), Cycle(to));
    }
}

TEST(FormUseTest, ReadFormLine_ReadsNamesInOrder)
{
    const auto uses = readFormLine("intro verse verse outro");

    ASSERT_EQ(uses.size(), 4U);
    EXPECT_EQ(uses[0], (FormUse{.name = "intro", .bars = 0}));
    EXPECT_EQ(uses[1].name, "verse");
    EXPECT_EQ(uses[2].name, "verse");
    EXPECT_EQ(uses[3].name, "outro");
}

TEST(FormUseTest, ReadFormLine_ReadsAPerOccurrenceLength)
{
    const auto uses = readFormLine("verse/4 chorus");

    ASSERT_EQ(uses.size(), 2U);
    EXPECT_EQ(uses[0], (FormUse{.name = "verse", .bars = 4}));
    EXPECT_EQ(uses[1], (FormUse{.name = "chorus", .bars = 0}));
}

TEST(FormUseTest, ReadFormLine_SplitsOnTabsToo)
{
    EXPECT_EQ(readFormLine("a\tb_2\tC9").size(), 3U);
}

TEST(FormUseTest, ReadFormLine_RefusesAnEmptyLine)
{
    EXPECT_THROW((void)readFormLine(""), ScoreError);
    EXPECT_THROW((void)readFormLine("  \t "), ScoreError);
}

TEST(FormUseTest, ReadFormLine_RefusesANameOutsideItsAlphabet)
{
    EXPECT_THROW((void)readFormLine("ver!se"), ScoreError);
    EXPECT_THROW((void)readFormLine("ver~se"), ScoreError);
}

TEST(FormUseTest, OperatorEquals_ComparesOccurrencesFieldByField)
{
    const FormUse use{.name = "verse", .bars = 8};

    EXPECT_EQ(use, (FormUse{.name = "verse", .bars = 8}));
    EXPECT_NE(use, (FormUse{.name = "chorus", .bars = 8}));
    EXPECT_NE(use, (FormUse{.name = "verse", .bars = 4}));
}

TEST(FormUseTest, ReadFormLine_RefusesALengthWithNoName)
{
    EXPECT_THROW((void)readFormLine("/4"), ScoreError);
}

TEST(FormUseTest, ReadFormLine_RefusesALengthThatIsNotANumber)
{
    EXPECT_THROW((void)readFormLine("verse/"), ScoreError);
    EXPECT_THROW((void)readFormLine("verse/4x"), ScoreError);
    EXPECT_THROW(
        (void)readFormLine("verse/99999999999999999999"), ScoreError);
}

TEST(FormUseTest, ReadFormLine_RefusesALengthOfNothing)
{
    EXPECT_THROW((void)readFormLine("verse/0"), ScoreError);
    EXPECT_THROW((void)readFormLine("verse/-1"), ScoreError);
}

TEST(FormUseTest, ReadFormLine_RefusesALengthPastTheLimit)
{
    EXPECT_EQ(readFormLine("verse/1024")[0].bars, 1024);
    EXPECT_THROW((void)readFormLine("verse/1025"), ScoreError);
}

TEST(FormUseTest, ReadBarsLine_ReadsOneNumber)
{
    EXPECT_EQ(readBarsLine("8"), 8);
    EXPECT_EQ(readBarsLine(" 16\t"), 16);
}

TEST(FormUseTest, ReadBarsLine_RefusesAnythingButOneNumber)
{
    EXPECT_THROW((void)readBarsLine(""), ScoreError);
    EXPECT_THROW((void)readBarsLine("8 9"), ScoreError);
    EXPECT_THROW((void)readBarsLine("eight"), ScoreError);
    EXPECT_THROW((void)readBarsLine("0"), ScoreError);
    EXPECT_THROW((void)readBarsLine("1025"), ScoreError);
}

TEST(FormUseTest, ReadPartLine_ReadsNamesAndDropsDuplicates)
{
    const auto names = readPartLine("verse bridge verse");

    ASSERT_EQ(names.size(), 2U);
    EXPECT_EQ(names[0], "verse");
    EXPECT_EQ(names[1], "bridge");
}

TEST(FormUseTest, ReadPartLine_RefusesAnEmptyLine)
{
    EXPECT_THROW((void)readPartLine(""), ScoreError);
}

TEST(FormUseTest, ReadPartLine_RefusesANameWithALength)
{
    EXPECT_THROW((void)readPartLine("verse/4"), ScoreError);
}

TEST(FormUseTest, ResolveBars_FillsUnmarkedFromTheDefaultAlone)
{
    std::vector<FormUse> uses{
        FormUse{.name = "a", .bars = 0},
        FormUse{.name = "b", .bars = 2}};

    resolveBars(uses, 8);

    EXPECT_EQ(uses[0].bars, 8);
    EXPECT_EQ(uses[1].bars, 2);
}

TEST(FormUseTest, ResolveBars_RefusesAnUnmarkedNameWithNoDefault)
{
    std::vector<FormUse> uses{FormUse{.name = "a", .bars = 0}};

    EXPECT_THROW(resolveBars(uses, 0), ScoreError);
}

TEST(FormUseTest, PeriodOf_SumsEveryOccurrence)
{
    EXPECT_EQ(
        periodOf(
            {FormUse{.name = "a", .bars = 8},
             FormUse{.name = "b", .bars = 4},
             FormUse{.name = "a", .bars = 8}}),
        20);
}

TEST(FormUseTest, WindowsFor_PicksEveryOccurrenceOfItsNames)
{
    const std::vector<FormUse> uses{
        FormUse{.name = "intro", .bars = 2},
        FormUse{.name = "verse", .bars = 8},
        FormUse{.name = "bridge", .bars = 4},
        FormUse{.name = "verse", .bars = 8}};

    const auto windows = windowsFor({"verse", "bridge"}, uses);

    ASSERT_EQ(windows.size(), 3U);
    EXPECT_EQ(windows[0], cycles(2, 10));
    EXPECT_EQ(windows[1], cycles(10, 14));
    EXPECT_EQ(windows[2], cycles(14, 22));
}

TEST(FormUseTest, WindowsFor_IsEmptyWhenTheFormNeverPlaysThem)
{
    const std::vector<FormUse> uses{
        FormUse{.name = "verse", .bars = 8}};

    EXPECT_TRUE(windowsFor({"chorus"}, uses).empty());
}
