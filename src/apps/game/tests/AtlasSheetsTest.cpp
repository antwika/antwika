#include <gtest/gtest.h>

#include <array>
#include <cstddef>

#include <antwika/atlas/AtlasMeta.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/GfxError.hpp>
#include <antwika/gfx/Rect.hpp>

#include "AtlasSpecsFixture.hpp"
#include "antwika/game/AtlasSheets.hpp"
#include "antwika/game/GameConfig.hpp"
#include "antwika/game/TileAtlas.hpp"

using antwika::atlas::AtlasMeta;
using antwika::game::AtlasKind;
using antwika::game::AtlasAssets;
using antwika::game::atlasMetaAsset;
using antwika::game::loadAtlasSheets;
using antwika::game::requireAtlasSize;
using antwika::game::AtlasSpec;
using antwika::game::AtlasSpecs;
using antwika::game::kAtlasKindCount;
using antwika::game::requireAtlasSpecs;
using antwika::game::specFrom;
using antwika::game::specsFrom;
using antwika::game::testing::kTestSpecs;
using antwika::gfx::GfxError;
using antwika::gfx::Size;

namespace
{
    const AtlasAssets kShipped = antwika::game::defaultAtlases();

    [[nodiscard]] AtlasSpecs shippedSpecs()
    {
        std::array<AtlasMeta, kAtlasKindCount> metas{};

        for (std::size_t index = 0; index < kAtlasKindCount; ++index)
        {
            metas[index] = atlasMetaAsset(kShipped.byKind[index]);
        }

        return specsFrom(metas, atlasMetaAsset(kShipped.walker));
    }

    [[nodiscard]] AtlasSpecs whereTheWalkersAre(const AtlasSpec &spec)
    {
        auto specs = kTestSpecs;
        specs.walker = spec;

        return specs;
    }

    [[nodiscard]] AtlasSpecs whereOneByOneIs(const AtlasSpec &spec)
    {
        auto specs = kTestSpecs;
        specs.byKind[0] = spec;

        return specs;
    }

    [[nodiscard]] AtlasSpecs whereTwoByTwoIs(const AtlasSpec &spec)
    {
        auto specs = kTestSpecs;
        specs.byKind[1] = spec;

        return specs;
    }
}

TEST(AtlasSheetsTest, AtlasMetaAsset_ReadsWhatEverySheetRecords)
{
    EXPECT_EQ(shippedSpecs(), kTestSpecs);
}

TEST(AtlasSheetsTest, AtlasMetaAsset_RefusesASheetWithNoSidecar)
{
    EXPECT_THROW(
        [[maybe_unused]] const auto meta = atlasMetaAsset("absent.png"),
        GfxError);
}

TEST(AtlasSheetsTest, RequireAtlasSpecs_AcceptsWhatTheSheetsRecord)
{
    EXPECT_NO_THROW(requireAtlasSpecs(shippedSpecs()));
}

TEST(AtlasSheetsTest, SpecFrom_CarriesEveryFactTheMetadataNames)
{
    const AtlasMeta meta{
        .kind = antwika::atlas::AtlasKind::Isometric,
        .columns = 4,
        .rows = 3,
        .sprite = {.width = 64, .height = 96},
        .pivot = {.x = 32, .y = 64},
        .isometric = {.width = 32, .height = 16}};

    EXPECT_EQ(
        specFrom(meta),
        (AtlasSpec{
            .spriteSize = {.width = 64, .height = 96},
            .pivot = {.x = 32, .y = 64},
            .isometric = {.width = 32, .height = 16},
            .columns = 4,
            .rows = 3}));
}

TEST(AtlasSheetsTest, RequireAtlasSpecs_RefusesASheetWithNoSlots)
{
    auto spec = kTestSpecs.of(AtlasKind::OneByOne);
    spec.columns = 0;

    EXPECT_THROW(
        requireAtlasSpecs(whereOneByOneIs(spec)), GfxError);
}

TEST(AtlasSheetsTest, RequireAtlasSpecs_RefusesAWalkerSheetWithNoSlots)
{
    auto spec = kTestSpecs.walker;
    spec.columns = 0;

    EXPECT_THROW(
        requireAtlasSpecs(whereTheWalkersAre(spec)), GfxError);
}

TEST(AtlasSheetsTest, RequireAtlasSpecs_RefusesTooFewRowsForTheWalkers)
{
    auto spec = kTestSpecs.walker;
    spec.rows = 3;

    EXPECT_THROW(
        requireAtlasSpecs(whereTheWalkersAre(spec)), GfxError);
}

TEST(AtlasSheetsTest, RequireAtlasSpecs_RefusesTooFewColumnsToWalk)
{
    auto spec = kTestSpecs.walker;
    spec.columns = 3;
    spec.rows = 32;

    EXPECT_THROW(
        requireAtlasSpecs(whereTheWalkersAre(spec)), GfxError);
}

TEST(AtlasSheetsTest, RequireAtlasSpecs_RefusesAWalkerFootprintOfAnotherSize)
{
    auto spec = kTestSpecs.walker;
    spec.isometric.width = 64;

    EXPECT_THROW(
        requireAtlasSpecs(whereTheWalkersAre(spec)), GfxError);
}

TEST(AtlasSheetsTest, RequireAtlasSpecs_RefusesAWalkerMetricThatCannotHalve)
{
    auto spec = kTestSpecs.walker;
    spec.pivot.y += 1;

    EXPECT_THROW(
        requireAtlasSpecs(whereTheWalkersAre(spec)), GfxError);
}

TEST(AtlasSheetsTest, RequireAtlasSpecs_RefusesASheetShortOfBuildings)
{
    auto spec = kTestSpecs.of(AtlasKind::TwoByTwo);
    spec.columns = 1;
    spec.rows = 3;

    EXPECT_THROW(
        requireAtlasSpecs(whereTwoByTwoIs(spec)), GfxError);
}

TEST(AtlasSheetsTest, RequireAtlasSpecs_RefusesASheetShortOfDebris)
{
    auto spec = kTestSpecs.of(AtlasKind::TwoByTwo);
    spec.columns = 2;
    spec.rows = 4;

    EXPECT_THROW(
        requireAtlasSpecs(whereTwoByTwoIs(spec)), GfxError);
}

TEST(AtlasSheetsTest, RequireAtlasSpecs_RefusesASheetShortOfFire)
{
    auto spec = kTestSpecs.of(AtlasKind::TwoByTwo);
    spec.columns = 3;
    spec.rows = 3;

    EXPECT_THROW(
        requireAtlasSpecs(whereTwoByTwoIs(spec)), GfxError);
}

TEST(AtlasSheetsTest, RequireAtlasSpecs_RefusesAFootprintOfAnotherSize)
{
    auto wide = kTestSpecs.of(AtlasKind::OneByOne);
    wide.isometric.width = 64;

    EXPECT_THROW(requireAtlasSpecs(whereOneByOneIs(wide)), GfxError);

    auto tall = kTestSpecs.of(AtlasKind::OneByOne);
    tall.isometric.height = 32;

    EXPECT_THROW(requireAtlasSpecs(whereOneByOneIs(tall)), GfxError);
}

TEST(AtlasSheetsTest, RequireAtlasSpecs_RefusesAMetricThatCannotHalve)
{
    for (std::size_t metric = 0; metric < 4; ++metric)
    {
        auto spec = kTestSpecs.of(AtlasKind::OneByOne);

        switch (metric)
        {
        case 0:
            spec.spriteSize.width += 1;
            break;
        case 1:
            spec.spriteSize.height += 1;
            break;
        case 2:
            spec.pivot.x += 1;
            break;
        default:
            spec.pivot.y += 1;
            break;
        }

        EXPECT_THROW(
            requireAtlasSpecs(whereOneByOneIs(spec)), GfxError)
            << metric;
    }
}

TEST(AtlasSheetsTest, SpriteRect_HoldsNoAreaOnASheetWithNoSlots)
{
    auto spec = kTestSpecs.of(AtlasKind::OneByOne);
    spec.columns = 0;

    EXPECT_EQ(
        antwika::game::spriteRect(
            whereOneByOneIs(spec), AtlasKind::OneByOne, 0),
        antwika::gfx::Rect{});
}

TEST(AtlasSheetsTest, OperatorEquals_ComparesEveryMetricASpecHolds)
{
    const auto spec = kTestSpecs.of(AtlasKind::OneByOne);

    EXPECT_EQ(spec, kTestSpecs.of(AtlasKind::OneByOne));

    for (std::size_t metric = 0; metric < 6; ++metric)
    {
        auto changed = spec;

        switch (metric)
        {
        case 0:
            changed.spriteSize.width += 1;
            break;
        case 1:
            changed.spriteSize.height += 1;
            break;
        case 2:
            changed.pivot.x += 1;
            break;
        case 3:
            changed.isometric.width += 1;
            break;
        case 4:
            changed.columns += 1;
            break;
        default:
            changed.rows += 1;
            break;
        }

        EXPECT_NE(spec, changed) << metric;
    }
}

TEST(AtlasSheetsTest, OperatorEquals_ComparesEverySheetItGathers)
{
    EXPECT_EQ(kTestSpecs, shippedSpecs());

    for (std::size_t sheet = 0; sheet < kAtlasKindCount; ++sheet)
    {
        auto specs = kTestSpecs;
        specs.byKind[sheet].columns += 1;

        EXPECT_NE(kTestSpecs, specs) << sheet;
    }

    auto walking = kTestSpecs;
    walking.walker.columns += 1;

    EXPECT_NE(kTestSpecs, walking);
}

TEST(AtlasSheetsTest, LoadAtlasSheets_ReadsEverySheetAndItsMetadata)
{
    const auto sheets = loadAtlasSheets(kShipped);

    EXPECT_EQ(sheets.specs, kTestSpecs);

    for (std::size_t index = 0; index < kAtlasKindCount; ++index)
    {
        const auto kind = static_cast<AtlasKind>(index);

        EXPECT_EQ(
            sheets.of(kind).size, kTestSpecs.of(kind).sheetSize())
            << kShipped.byKind[index];
    }

    EXPECT_EQ(sheets.walker.size, kTestSpecs.walker.sheetSize());
}

TEST(AtlasSheetsTest, RequireAtlasSize_AcceptsWhatTheMetadataRecords)
{
    for (std::size_t index = 0; index < kAtlasKindCount; ++index)
    {
        const auto &spec = kTestSpecs.byKind[index];

        EXPECT_NO_THROW(requireAtlasSize(
            antwika::gfx::Bitmap{.size = spec.sheetSize(), .pixels = {}},
            spec,
            kShipped.byKind[index]));
    }
}

TEST(AtlasSheetsTest, RequireAtlasSize_RefusesAnImageOfAnotherSize)
{
    const auto &spec = kTestSpecs.of(AtlasKind::OneByOne);
    auto wrong = spec.sheetSize();
    wrong.width -= 1;

    EXPECT_THROW(
        requireAtlasSize(
            antwika::gfx::Bitmap{.size = wrong, .pixels = {}},
            spec,
            "atlas_1x1.png"),
        GfxError);
}

TEST(AtlasSheetsTest, RequireAtlasSize_RefusesAnEmptyImage)
{
    EXPECT_THROW(
        requireAtlasSize(
            antwika::gfx::Bitmap{},
            kTestSpecs.of(AtlasKind::OneByOne),
            "atlas_1x1.png"),
        GfxError);
}

TEST(AtlasSheetsTest, LoadAtlasSheets_RefusesASheetThatIsNotThere)
{
    auto absent = kShipped;
    absent.byKind[0] = "absent.png";

    EXPECT_THROW(
        [[maybe_unused]] const auto sheets = loadAtlasSheets(absent),
        GfxError);
}

TEST(AtlasSheetsTest, LoadAtlasSheets_RefusesAWalkerSheetThatIsNotThere)
{
    auto absent = kShipped;
    absent.walker = "absent.png";

    EXPECT_THROW(
        [[maybe_unused]] const auto sheets = loadAtlasSheets(absent),
        GfxError);
}
