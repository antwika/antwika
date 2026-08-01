#include <gtest/gtest.h>

#include <string>
#include <vector>

#include <antwika/cli/CommandLine.hpp>
#include <antwika/cli/CommandLineError.hpp>

#include "antwika/sound_demo/DemoOptions.hpp"

using antwika::cli::CommandLineError;
using antwika::sound_demo::demoFlags;
using antwika::sound_demo::DemoOptions;
using antwika::sound_demo::demoOptionsFrom;

namespace
{
    // This demo parses its own table rather than a concatenated one.
    // It records nothing, so there are no replay flags to append.
    // Not [[nodiscard]]: half of these cases call it to be refused.
    DemoOptions parse(std::vector<const char *> args)
    {
        std::vector<char *> argv;
        argv.reserve(args.size());
        for (const auto *arg : args)
        {
            argv.push_back(const_cast<char *>(arg));
        }

        return demoOptionsFrom(antwika::cli::parseCommandLine(
            static_cast<int>(argv.size()), argv.data(), demoFlags()));
    }

    TEST(DemoOptionsTest, NoFlags_PlaysTheGeneratedTone)
    {
        const auto options = parse({"antwika_sound_demo"});

        EXPECT_FALSE(options.filePath.has_value());
        EXPECT_FALSE(options.helpRequested);
    }

    TEST(DemoOptionsTest, File_NamesTheWavToPlay)
    {
        const auto options =
            parse({"antwika_sound_demo", "--file", "my.wav"});

        ASSERT_TRUE(options.filePath.has_value());
        EXPECT_EQ(*options.filePath, "my.wav");
    }

    TEST(DemoOptionsTest, Help_IsAskedFor)
    {
        const auto options = parse({"antwika_sound_demo", "--help"});

        EXPECT_TRUE(options.helpRequested);
        EXPECT_FALSE(options.filePath.has_value());
    }

    // The whole point of the flag, said as the failure it prevents.
    // A bare filename used to be taken and opened.
    // So did a mistyped flag, which failed inside the WAV reader.
    TEST(DemoOptionsTest, BareArgument_IsRefusedRatherThanOpened)
    {
        EXPECT_THROW(
            parse({"antwika_sound_demo", "my.wav"}), CommandLineError);
    }

    TEST(DemoOptionsTest, MistypedFlag_IsRefused)
    {
        EXPECT_THROW(
            parse({"antwika_sound_demo", "--flie", "my.wav"}),
            CommandLineError);
    }

    TEST(DemoOptionsTest, File_WithNoValueIsRefused)
    {
        EXPECT_THROW(
            parse({"antwika_sound_demo", "--file"}), CommandLineError);
    }

    // One table is the parser's input and the help text's.
    // So a flag that parses but is undocumented is not expressible.
    TEST(DemoOptionsTest, Flags_AreDocumented)
    {
        ASSERT_EQ(demoFlags().size(), 1U);
        EXPECT_EQ(demoFlags().front().name, "--file");
        EXPECT_FALSE(demoFlags().front().help.empty());
    }
} // namespace
