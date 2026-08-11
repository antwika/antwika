#include <gtest/gtest.h>

#include <filesystem>

#include <antwika/geometry/Grid.hpp>
#include <antwika/testing/ScratchPath.hpp>
#include <antwika/tilemap/MapFile.hpp>
#include <antwika/tilemap/MapHeader.hpp>
#include <antwika/tilemap/MapJson.hpp>
#include <antwika/tilemap/Slab.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/tilemap/TileMap.hpp>
#include <antwika/tilemap/TileMapError.hpp>

using antwika::geometry::GridCell;
using antwika::testing::ScratchDirectory;
using antwika::tilemap::loadMapDocumentFile;
using antwika::tilemap::loadMapFile;
using antwika::tilemap::MapDocument;
using antwika::tilemap::MapHeader;
using antwika::tilemap::saveMapFile;
using antwika::tilemap::Slab;
using antwika::tilemap::TerrainClass;
using antwika::tilemap::TileMap;
using antwika::tilemap::TileMapError;

namespace
{
    /**
     * @brief A file that opens for writing and fails every write.
     */
    constexpr auto kFullDevice = "/dev/full";

    [[nodiscard]] TileMap markedMap()
    {
        TileMap map(MapHeader{.id = "wakewater-07"}, 3, 2);

        (void)map.at(GridCell{.column = 2, .row = 1})
            .place(Slab{.level = 4, .terrain = TerrainClass::Cliff});

        return map;
    }

    /**
     * @brief A path inside a directory that does not exist.
     */
    [[nodiscard]] std::filesystem::path unopenablePath(
        const ScratchDirectory &scratch)
    {
        return scratch.path() / "absent" / "map.json";
    }
}

TEST(MapFileTest, SaveMapFile_RoundTripsAMapThroughTheFile)
{
    const ScratchDirectory scratch("mapfile.");
    const auto where = scratch.path() / "map.json";

    saveMapFile(where, markedMap());

    const auto loaded = loadMapFile(where);

    EXPECT_EQ(loaded.header().id, "wakewater-07");
    EXPECT_EQ(loaded.columns(), 3U);
    EXPECT_EQ(loaded.rows(), 2U);
    ASSERT_NE(
        loaded.at(GridCell{.column = 2, .row = 1}).slabAt(4), nullptr);
    EXPECT_EQ(
        loaded.at(GridCell{.column = 2, .row = 1})
            .slabAt(4)
            ->terrain,
        TerrainClass::Cliff);
}

TEST(MapFileTest, SaveMapFile_ThrowsWhenTheMapFileCannotBeOpened)
{
    const ScratchDirectory scratch("mapfile.");

    EXPECT_THROW(
        saveMapFile(unopenablePath(scratch), markedMap()),
        TileMapError);
}

TEST(MapFileTest, SaveMapFile_NamesThePathItCouldNotOpen)
{
    const ScratchDirectory scratch("mapfile.");
    const auto where = unopenablePath(scratch);

    try
    {
        saveMapFile(where, markedMap());
        FAIL() << "saveMapFile accepted an unopenable path";
    }
    catch (const TileMapError &error)
    {
        EXPECT_NE(
            std::string(error.what()).find(where.string()),
            std::string::npos);
    }
}

TEST(MapFileTest, LoadMapFile_ThrowsWhenTheMapFileIsAbsent)
{
    const ScratchDirectory scratch("mapfile.");

    EXPECT_THROW(
        (void)loadMapFile(scratch.path() / "absent.json"),
        TileMapError);
}

TEST(MapFileTest, LoadMapFile_ThrowsWhenTheMapFileIsNotValidJson)
{
    const ScratchDirectory scratch("mapfile.");
    scratch.write("broken.json", "{ this is not json");

    EXPECT_THROW(
        (void)loadMapFile(scratch.path() / "broken.json"),
        TileMapError);
}

TEST(MapFileTest, LoadMapFile_SaysTheMapFileIsNotValidJson)
{
    const ScratchDirectory scratch("mapfile.");
    scratch.write("broken.json", "{ this is not json");

    try
    {
        (void)loadMapFile(scratch.path() / "broken.json");
        FAIL() << "loadMapFile accepted a file that is not json";
    }
    catch (const TileMapError &error)
    {
        EXPECT_NE(
            std::string(error.what()).find("not valid json"),
            std::string::npos);
    }
}

TEST(MapFileTest, SaveMapFile_RoundTripsADocumentWithItsFreeMask)
{
    const ScratchDirectory scratch("mapfile.");
    const auto where = scratch.path() / "document.json";

    const MapDocument document{
        .map = markedMap(),
        .free = {true, false, true, false, true, false}};

    saveMapFile(where, document);

    const auto loaded = loadMapDocumentFile(where);

    EXPECT_EQ(loaded.map.header().id, "wakewater-07");
    EXPECT_EQ(loaded.free, document.free);
}

TEST(MapFileTest, SaveMapFile_ThrowsWhenTheDocumentCannotBeOpened)
{
    const ScratchDirectory scratch("mapfile.");
    const MapDocument document{.map = markedMap(), .free = {}};

    EXPECT_THROW(
        saveMapFile(unopenablePath(scratch), document), TileMapError);
}

TEST(MapFileTest, LoadMapDocumentFile_ThrowsWhenTheFileIsAbsent)
{
    const ScratchDirectory scratch("mapfile.");

    EXPECT_THROW(
        (void)loadMapDocumentFile(scratch.path() / "absent.json"),
        TileMapError);
}

TEST(MapFileTest, LoadMapDocumentFile_ThrowsWhenTheFileIsNotJson)
{
    const ScratchDirectory scratch("mapfile.");
    scratch.write("broken.json", "[[[");

    EXPECT_THROW(
        (void)loadMapDocumentFile(scratch.path() / "broken.json"),
        TileMapError);
}

TEST(MapFileTest, SaveMapFile_ThrowsWhenTheMapCannotBeWritten)
{
    if (!std::filesystem::exists(kFullDevice))
    {
        GTEST_SKIP() << "no " << kFullDevice << " to fill up";
    }

    EXPECT_THROW(saveMapFile(kFullDevice, markedMap()), TileMapError);
}

TEST(MapFileTest, SaveMapFile_ThrowsWhenTheDocumentCannotBeWritten)
{
    if (!std::filesystem::exists(kFullDevice))
    {
        GTEST_SKIP() << "no " << kFullDevice << " to fill up";
    }

    const MapDocument document{.map = markedMap(), .free = {}};

    EXPECT_THROW(saveMapFile(kFullDevice, document), TileMapError);
}
