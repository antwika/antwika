#include <gtest/gtest.h>

#include <antwika/game/TileAtlas.hpp>

#include "antwika/atlas_editor/AtlasEditor.hpp"

/**
 * @file
 * @brief The one place this editor is held to the game's own contract.
 *
 * This application serves exactly one sheet, and `game::requireAtlasSize`
 * refuses any size but `game::kAtlasSize` at startup -- so a session
 * started blank here, painted and saved is a file the game will not open
 * unless the two numbers agree.
 * The whole of what this file adds is that they cannot disagree quietly:
 * a row added to `game::kAtlasRows` is a red build here rather than a
 * refusal somebody meets much later, in another application, holding an
 * afternoon's art.
 *
 * It is the only thing under `src/apps/atlas_editor/` that names
 * `apps/game`, and it names one constant of it.
 * Nothing this editor *builds* depends on the game: the dependency is the
 * test target's alone, which is what keeps the two applications separate
 * everywhere the contract is not the subject.
 */
namespace
{
    using antwika::atlas_editor::kDefaultSheetSize;
    using antwika::game::AtlasKind;

    // The game's 1x1 sheet, being the one a session starts blank on.
    constexpr auto kGameSheet =
        antwika::game::atlasSizeOf(AtlasKind::OneByOne);

    // A static_assert as well as a case, deliberately.
    // Both constants are compile-time, so the build can answer it.
    // And a red build is louder than a red test.
    static_assert(
        kDefaultSheetSize.width == kGameSheet.width
        && kDefaultSheetSize.height == kGameSheet.height,
        "atlas_editor::kDefaultSheetSize must be the game's 1x1 sheet: "
        "a blank sheet this editor opens is one the game has to accept");

    TEST(DefaultSheetSizeTest, TheBlankSheetIsTheSizeTheGameDemands)
    {
        EXPECT_EQ(kDefaultSheetSize, kGameSheet);
    }

    // Stated in sprites as well as in pixels.
    // So a failure names which of the two moved rather than that one did.
    TEST(DefaultSheetSizeTest, TheBlankSheetIsAWholeNumberOfSlots)
    {
        const auto sprite =
            antwika::game::atlasSpec(AtlasKind::OneByOne).spriteSize;

        EXPECT_EQ(
            kDefaultSheetSize.width,
            antwika::game::kAtlasColumns * sprite.width);
        EXPECT_EQ(
            kDefaultSheetSize.height,
            antwika::game::kAtlasRows * sprite.height);
    }
} // namespace
