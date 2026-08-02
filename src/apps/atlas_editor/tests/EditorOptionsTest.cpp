#include <gtest/gtest.h>

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>

#include <antwika/cli/CommandLine.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/atlas_editor/AtlasEditor.hpp"
#include "antwika/atlas_editor/EditorOptions.hpp"
#include "antwika/atlas_editor/TileGrid.hpp"

using antwika::atlas_editor::editorFlags;
using antwika::atlas_editor::EditorOptions;
using antwika::atlas_editor::editorOptionsFrom;
using antwika::atlas_editor::kDefaultMaxTicks;
using antwika::atlas_editor::kDefaultSheetSize;
using antwika::atlas_editor::parseSize;
using antwika::atlas_editor::TileGrid;
using antwika::cli::CommandLine;
using antwika::gfx::Size;

namespace
{
    EditorOptions optionsFrom(CommandLine::Values given)
    {
        return editorOptionsFrom(CommandLine(std::move(given)));
    }

    bool accepts(const std::string_view flag)
    {
        const auto flags = editorFlags();

        return std::any_of(
            flags.begin(),
            flags.end(),
            [flag](const auto &spec) { return spec.name == flag; });
    }
} // namespace

TEST(ParseSizeTest, ParseSize_ReadsTwoNumbersWithACrossBetweenThem)
{
    EXPECT_EQ(parseSize("1024x256"), (Size{.width = 1024, .height = 256}));
}

TEST(ParseSizeTest, ParseSize_RefusesAnythingElse)
{
    EXPECT_FALSE(parseSize("1024").has_value());

    // 4294967297 is one past 32 bits with its low bits reading 1.
    // The old cast silently opened a 1x64 sheet for that typo.
    EXPECT_FALSE(parseSize("4294967297x64").has_value());
    EXPECT_FALSE(parseSize("64x4294967297").has_value());
    EXPECT_FALSE(parseSize("axb").has_value());
    EXPECT_FALSE(parseSize("12xb").has_value());
    EXPECT_FALSE(parseSize("12x").has_value());
    EXPECT_FALSE(parseSize("0x8").has_value());
    EXPECT_FALSE(parseSize("8x0").has_value());
    EXPECT_FALSE(parseSize("8x8 ").has_value());
}

TEST(EditorOptionsTest, EditorFlags_DocumentEveryFlagItParses)
{
    EXPECT_TRUE(accepts("--image"));
    EXPECT_TRUE(accepts("--out"));
    EXPECT_TRUE(accepts("--sheet"));
    EXPECT_TRUE(accepts("--tile"));
    EXPECT_TRUE(accepts("--max-ticks"));
}

TEST(EditorOptionsTest, EditorOptionsFrom_OpensTheGameSheetByDefault)
{
    const EditorOptions options = optionsFrom({});

    EXPECT_FALSE(options.imagePath.has_value());
    EXPECT_FALSE(options.outPath.has_value());
    EXPECT_EQ(options.sheet, kDefaultSheetSize);
    EXPECT_EQ(options.tile, TileGrid{});
    EXPECT_EQ(options.maxTicks, kDefaultMaxTicks);
}

TEST(EditorOptionsTest, EditorOptionsFrom_TakesEveryFlagItWasGiven)
{
    const EditorOptions options = optionsFrom(
        {{"--image", "atlas.png"},
         {"--out", "mine.png"},
         {"--sheet", "64x32"},
         {"--tile", "16x8"},
         {"--max-ticks", "120"}});

    EXPECT_EQ(options.imagePath, std::string("atlas.png"));
    EXPECT_EQ(options.outPath, std::string("mine.png"));
    EXPECT_EQ(options.sheet, (Size{.width = 64, .height = 32}));
    EXPECT_EQ(options.tile, (TileGrid{.width = 16, .height = 8}));
    EXPECT_EQ(options.maxTicks, 120U);
}

TEST(EditorOptionsTest, EditorOptionsFrom_KeepsADefaultAValueSpoiled)
{
    const EditorOptions options = optionsFrom(
        {{"--sheet", "wide"},
         {"--tile", "big"},
         {"--max-ticks", "soon"}});

    EXPECT_EQ(options.sheet, kDefaultSheetSize);
    EXPECT_EQ(options.tile, TileGrid{});
    EXPECT_EQ(options.maxTicks, kDefaultMaxTicks);
}

// Nought ticks is how somebody at a real window asks for no cap.
TEST(EditorOptionsTest, EditorOptionsFrom_ReadsZeroTicksAsNoCapAtAll)
{
    const EditorOptions options = optionsFrom({{"--max-ticks", "0"}});

    EXPECT_FALSE(options.maxTicks.has_value());
}
