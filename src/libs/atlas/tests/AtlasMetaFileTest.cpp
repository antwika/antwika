#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <string>

#include <antwika/testing/ScratchPath.hpp>

#include "antwika/atlas/AtlasError.hpp"
#include "antwika/atlas/AtlasMeta.hpp"
#include "antwika/atlas/AtlasMetaFile.hpp"

using antwika::atlas::AtlasError;
using antwika::atlas::AtlasKind;
using antwika::atlas::AtlasMeta;
using antwika::atlas::loadMetaFile;
using antwika::atlas::metaFromJson;
using antwika::atlas::metaPathFor;
using antwika::atlas::metaToJson;
using antwika::atlas::storeMetaFile;
using antwika::gfx::Point;
using antwika::gfx::Size;
using antwika::testing::ScratchDirectory;

namespace
{
    AtlasMeta described()
    {
        return AtlasMeta{
            .kind = AtlasKind::Isometric,
            .columns = 8,
            .rows = 6,
            .sprite = {.width = 64, .height = 96},
            .pivot = {.x = 32, .y = 64},
            .isometric = {.width = 32, .height = 16}};
    }
}

TEST(AtlasMetaFileTest, MetaPathFor_HangsTheSidecarOffTheImageName)
{
    EXPECT_EQ(metaPathFor("tiles/city.png"), "tiles/city.png.json");
}

TEST(AtlasMetaFileTest, MetaFromJson_ReadsBackEveryFactItWasGiven)
{
    EXPECT_EQ(metaFromJson(metaToJson(described())), described());
}

TEST(AtlasMetaFileTest, MetaFromJson_ReadsBackAFlatAtlasAsFlat)
{
    auto flat = described();
    flat.kind = AtlasKind::Flat;

    EXPECT_EQ(metaFromJson(metaToJson(flat)), flat);
}

TEST(AtlasMetaFileTest, MetaFromJson_FallsBackWhereTheDocumentIsSilent)
{
    auto document = metaToJson(described());
    document.erase("kind");
    document.erase("columns");

    const auto read = metaFromJson(document);

    EXPECT_EQ(read.kind, AtlasKind::Isometric);
    EXPECT_EQ(read.columns, AtlasMeta{}.columns);
    EXPECT_EQ(read.rows, 6U);
}

TEST(AtlasMetaFileTest, MetaFromJson_RefusesAKindThisBuildDoesNotKnow)
{
    auto document = metaToJson(described());
    document["kind"] = "hexagonal";

    EXPECT_THROW(
        [[maybe_unused]] const auto read = metaFromJson(document),
        AtlasError);
}

TEST(AtlasMetaFileTest, MetaFromJson_RefusesASpriteWiderThanTheLimit)
{
    auto document = metaToJson(described());
    document["spriteWidth"] = 1 << 20;

    EXPECT_THROW(
        [[maybe_unused]] const auto read = metaFromJson(document),
        AtlasError);
}

TEST(AtlasMetaFileTest, LoadMetaFile_ReadsBackWhatStoreMetaFileWrote)
{
    const ScratchDirectory scratch("atlas-meta");
    const std::string path =
        (scratch.path() / "sheet.png.json").string();

    storeMetaFile(described(), path);

    EXPECT_EQ(loadMetaFile(path), described());
}

TEST(AtlasMetaFileTest, LoadMetaFile_AnswersNothingWhereThereIsNoFile)
{
    const ScratchDirectory scratch("atlas-meta");
    const std::string path =
        (scratch.path() / "absent.png.json").string();

    EXPECT_FALSE(loadMetaFile(path).has_value());
}

TEST(AtlasMetaFileTest, StoreMetaFile_RefusesADirectoryThatIsNotThere)
{
    const ScratchDirectory scratch("atlas-meta");
    const std::string path =
        (scratch.path() / "absent" / "sheet.png.json").string();

    EXPECT_THROW(storeMetaFile(described(), path), AtlasError);
}
