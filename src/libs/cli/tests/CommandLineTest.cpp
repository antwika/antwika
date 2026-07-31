#include <gtest/gtest.h>

#include <array>
#include <span>
#include <string>
#include <vector>

#include "antwika/cli/CommandLine.hpp"
#include "antwika/cli/CommandLineError.hpp"
#include "antwika/cli/FlagSpec.hpp"

using antwika::cli::CommandLine;
using antwika::cli::CommandLineError;
using antwika::cli::FlagSpec;
using antwika::cli::helpText;
using antwika::cli::kHelpFlag;
using antwika::cli::parseCommandLine;

namespace
{
    constexpr std::array kFlags{
        FlagSpec{
            .name = "--record",
            .valueName = "<path>",
            .help = "Write the run's events to <path>.",
        },
        FlagSpec{
            .name = "--verbose",
            .valueName = {},
            .help = "Say more about what is happening.",
        },
    };

    [[nodiscard]] CommandLine parse(std::vector<std::string> args)
    {
        std::vector<char *> argv;
        argv.reserve(args.size());
        for (auto &arg : args)
        {
            argv.push_back(arg.data());
        }
        return parseCommandLine(
            static_cast<int>(argv.size()),
            argv.data(),
            std::span<const FlagSpec>(kFlags));
    }
} // namespace

TEST(CommandLineTest, ParseFindsNothingInABareInvocation)
{
    const auto parsed = parse({"antwika_app"});

    EXPECT_FALSE(parsed.has("--record"));
    EXPECT_FALSE(parsed.value("--record").has_value());
}

TEST(CommandLineTest, ParseReadsAFlagsValue)
{
    const auto parsed = parse({"antwika_app", "--record", "out.json"});

    EXPECT_TRUE(parsed.has("--record"));
    EXPECT_EQ(parsed.value("--record"), "out.json");
}

TEST(CommandLineTest, ParseAcceptsAFlagThatTakesNoValue)
{
    const auto parsed = parse({"antwika_app", "--verbose"});

    EXPECT_TRUE(parsed.has("--verbose"));
    EXPECT_EQ(parsed.value("--verbose"), "");
}

// An edited shell history makes the last one the useful answer.
TEST(CommandLineTest, ParseKeepsTheLastOfARepeatedFlag)
{
    const auto parsed = parse(
        {"antwika_app", "--record", "first.json", "--record", "last.json"});

    EXPECT_EQ(parsed.value("--record"), "last.json");
}

// The defect this parser exists for.
// `--replya demo.json` used to start an empty session in silence.
TEST(CommandLineTest, ParseThrowsOnAFlagNoProgramKnows)
{
    EXPECT_THROW(
        (void)parse({"antwika_app", "--recrd", "out.json"}),
        CommandLineError);
}

TEST(CommandLineTest, ParseThrowsOnAnArgumentThatIsNotAFlagAtAll)
{
    EXPECT_THROW(
        (void)parse({"antwika_app", "out.json"}), CommandLineError);
}

TEST(CommandLineTest, ParseThrowsWhenAFlagIsMissingItsValue)
{
    try
    {
        (void)parse({"antwika_app", "--record"});
        FAIL() << "a flag with no value should have been refused";
    }
    catch (const CommandLineError &error)
    {
        const std::string message = error.what();
        EXPECT_NE(message.find("--record"), std::string::npos) << message;
        EXPECT_NE(message.find("needs a value"), std::string::npos)
            << message;
    }
}

// A value-less flag's value is not the argument after it.
TEST(CommandLineTest, ParseDoesNotSwallowWhatFollowsAValuelessFlag)
{
    EXPECT_THROW(
        (void)parse({"antwika_app", "--verbose", "out.json"}),
        CommandLineError);
}

TEST(CommandLineTest, ParseAcceptsHelpWithoutItBeingInTheTable)
{
    EXPECT_TRUE(parse({"antwika_app", "--help"}).has(kHelpFlag));
}

TEST(CommandLineTest, HelpTextNamesEveryFlagInTheTableAndHelpItself)
{
    const auto text =
        helpText("antwika_app", std::span<const FlagSpec>(kFlags));

    EXPECT_NE(text.find("Usage: antwika_app"), std::string::npos) << text;
    EXPECT_NE(text.find("--record <path>"), std::string::npos) << text;
    EXPECT_NE(
        text.find("Write the run's events to <path>."), std::string::npos)
        << text;
    EXPECT_NE(text.find("--verbose"), std::string::npos) << text;
    EXPECT_NE(text.find("--help"), std::string::npos) << text;
}

// A flag that takes no value is rendered without one.
TEST(CommandLineTest, HelpTextGivesAValuelessFlagNoValueName)
{
    const auto text =
        helpText("antwika_app", std::span<const FlagSpec>(kFlags));

    EXPECT_NE(text.find("--verbose  "), std::string::npos) << text;
}

TEST(CommandLineTest, HelpTextLinesUpTheDescriptions)
{
    const auto text =
        helpText("antwika_app", std::span<const FlagSpec>(kFlags));

    const auto first = text.find("Write the run's events");
    const auto second = text.find("Say more about");
    ASSERT_NE(first, std::string::npos) << text;
    ASSERT_NE(second, std::string::npos) << text;

    EXPECT_EQ(
        first - text.rfind('\n', first),
        second - text.rfind('\n', second));
}

TEST(CommandLineTest, HelpTextRendersAnEmptyTableAsJustHelp)
{
    const auto text = helpText("antwika_app", {});

    EXPECT_NE(text.find("--help"), std::string::npos) << text;
    EXPECT_EQ(text.find("--record"), std::string::npos) << text;
}
