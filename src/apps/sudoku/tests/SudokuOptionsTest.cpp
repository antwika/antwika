#include <gtest/gtest.h>

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <antwika/cli/CommandLine.hpp>
#include <antwika/sudoku/SudokuOptions.hpp>

using antwika::cli::CommandLine;
using antwika::sudoku::kDefaultMaxTicks;
using antwika::sudoku::SudokuOptions;
using antwika::sudoku::sudokuFlags;
using antwika::sudoku::sudokuOptionsFrom;

namespace
{
    [[nodiscard]] SudokuOptions optionsFrom(CommandLine::Values given)
    {
        return sudokuOptionsFrom(CommandLine(std::move(given)));
    }

    [[nodiscard]] bool accepts(const std::string_view flag)
    {
        const auto flags = sudokuFlags();

        return std::any_of(
            flags.begin(),
            flags.end(),
            [flag](const auto &spec) { return spec.name == flag; });
    }

    TEST(SudokuOptionsTest, SudokuFlags_DocumentEveryFlagItParses)
    {
        EXPECT_TRUE(accepts("--puzzle"));
        EXPECT_TRUE(accepts("--max-ticks"));

        EXPECT_FALSE(accepts("--locale"));

        for (const auto &spec : sudokuFlags())
        {
            EXPECT_FALSE(spec.help.empty());
            EXPECT_FALSE(spec.valueName.empty());
        }
    }

    TEST(SudokuOptionsTest, SudokuOptionsFrom_DefaultsToTheDemoAndACap)
    {
        const auto options = optionsFrom({});

        EXPECT_FALSE(options.puzzlePath.has_value());
        EXPECT_EQ(options.maxTicks, std::optional{kDefaultMaxTicks});
    }

    TEST(SudokuOptionsTest, SudokuOptionsFrom_ReadsWhatItIsGiven)
    {
        const auto options = optionsFrom(
            {{"--puzzle", "mine.txt"}, {"--max-ticks", "120"}});

        EXPECT_EQ(
            options.puzzlePath, std::optional<std::string>{"mine.txt"});
        EXPECT_EQ(options.maxTicks, std::optional<std::uint64_t>{120});
    }

    TEST(SudokuOptionsTest, SudokuOptionsFrom_ReadsZeroAsNoCapAtAll)
    {
        EXPECT_FALSE(
            optionsFrom({{"--max-ticks", "0"}}).maxTicks.has_value());
    }

    TEST(SudokuOptionsTest, SudokuOptionsFrom_KeepsTheDefaultOnATypo)
    {
        EXPECT_EQ(
            optionsFrom({{"--max-ticks", "soon"}}).maxTicks,
            std::optional{kDefaultMaxTicks});

        EXPECT_EQ(
            optionsFrom({{"--max-ticks", "12x"}}).maxTicks,
            std::optional{kDefaultMaxTicks});
    }
}
