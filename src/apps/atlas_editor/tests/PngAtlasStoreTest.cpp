#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

#include <unistd.h>

#include <antwika/atlas/AtlasMeta.hpp>
#include <antwika/atlas/AtlasMetaFile.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/GfxError.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/testing/ScratchPath.hpp>

#include "antwika/atlas_editor/AtlasEditorError.hpp"
#include "antwika/atlas_editor/PngAtlasStore.hpp"

using antwika::atlas_editor::AtlasEditorError;
using antwika::atlas_editor::PngAtlasStore;
using antwika::gfx::Bitmap;
using antwika::gfx::GfxError;
using antwika::gfx::Size;

namespace
{
    Bitmap twoByTwo()
    {
        return Bitmap{
            .size = {.width = 2, .height = 2},
            .pixels = std::vector<std::uint8_t>{
                255, 0,   0,   255, 0,  255, 0,   255,
                0,   0,   255, 0,   17, 34,  51,  128}};
    }
}

TEST(PngAtlasStoreTest, SaveThenLoad_GivesBackEveryByte)
{
    const antwika::testing::ScratchFile file(
        "antwika_atlas_editor_roundtrip.png");
    PngAtlasStore store(file.path(), file.path());

    store.save(twoByTwo());

    const auto loaded = store.load();

    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(*loaded, twoByTwo());
}

TEST(PngAtlasStoreTest, Load_AnswersNothingWhenThereIsNowhereToLoadFrom)
{
    PngAtlasStore store(std::nullopt, std::nullopt);

    EXPECT_FALSE(store.load().has_value());
}

TEST(PngAtlasStoreTest, Load_ThrowsWhenTheFileIsNotThere)
{
    PngAtlasStore store(
        std::string("/nonexistent/antwika/nothing.png"), std::nullopt);

    EXPECT_THROW(static_cast<void>(store.load()), GfxError);
}

TEST(PngAtlasStoreTest, Load_ThrowsWhenTheFileIsNotAPng)
{
    const antwika::testing::ScratchFile file(
        "antwika_atlas_editor_notapng.png");

    {
        std::ofstream out(file.path(), std::ios::binary);
        out << "this is not a png at all";
    }

    PngAtlasStore store(file.path(), std::nullopt);

    EXPECT_THROW(static_cast<void>(store.load()), GfxError);
}

TEST(PngAtlasStoreTest, Save_RefusesWhenThereIsNowhereToSaveTo)
{
    PngAtlasStore store(std::nullopt, std::nullopt);

    EXPECT_THROW(store.save(twoByTwo()), AtlasEditorError);
}

TEST(PngAtlasStoreTest, Save_ThrowsWhenTheFileCannotBeOpened)
{
    PngAtlasStore store(
        std::nullopt, std::string("/nonexistent/antwika/out.png"));

    EXPECT_THROW(store.save(twoByTwo()), GfxError);
}

TEST(PngAtlasStoreTest, SavePath_SaysWhereASaveWouldGo)
{
    const PngAtlasStore somewhere(
        std::nullopt, std::string("/tmp/out.png"));
    const PngAtlasStore nowhere(std::nullopt, std::nullopt);

    EXPECT_EQ(somewhere.savePath(), std::string("/tmp/out.png"));
    EXPECT_TRUE(nowhere.savePath().empty());
}

TEST(PngAtlasStoreTest, Save_WritesToItsOwnPathRatherThanTheOpenedOne)
{
    const antwika::testing::ScratchFile opened("antwika_atlas_editor_in.png");
    const antwika::testing::ScratchFile written("antwika_atlas_editor_out.png");

    PngAtlasStore seeding(std::nullopt, opened.path());
    seeding.save(twoByTwo());

    PngAtlasStore store(opened.path(), written.path());
    store.save(twoByTwo());

    EXPECT_TRUE(std::filesystem::exists(written.path()));
    EXPECT_EQ(store.savePath(), written.path());
}

TEST(PngAtlasStoreTest, SaveMetaThenLoadMeta_GivesBackEveryFact)
{
    const antwika::testing::ScratchFile file(
        "antwika_atlas_editor_meta.png");
    PngAtlasStore store(file.path(), file.path());

    const antwika::atlas::AtlasMeta meta{
        .kind = antwika::atlas::AtlasKind::Isometric,
        .columns = 4,
        .rows = 3,
        .sprite = {.width = 64, .height = 96},
        .pivot = {.x = 32, .y = 64},
        .isometric = {.width = 32, .height = 16}};

    store.saveMetaTo(meta, file.path());

    EXPECT_EQ(store.loadMetaFrom(file.path()), meta);
    EXPECT_TRUE(std::filesystem::exists(
        antwika::atlas::metaPathFor(file.path())));

    std::error_code ignored;
    std::filesystem::remove(
        antwika::atlas::metaPathFor(file.path()), ignored);
}

TEST(PngAtlasStoreTest, LoadMetaFrom_AnswersNothingWithNoSidecar)
{
    const antwika::testing::ScratchFile file(
        "antwika_atlas_editor_bare.png");
    PngAtlasStore store(file.path(), file.path());

    EXPECT_FALSE(store.loadMetaFrom(file.path()).has_value());
}
