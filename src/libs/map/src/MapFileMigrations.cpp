#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <exception>
#include <set>
#include <span>
#include <sstream>

#include <antwika/io/File.hpp>
#include <antwika/schema/JsonSchemas.hpp>
#include <antwika/schema/MigrationChain.hpp>
#include <antwika/schema/SchemaVersion.hpp>
#include <antwika/schema/Step.hpp>
#include <antwika/schema/VersionedDocument.hpp>
#include <antwika/tile/TilePaint.hpp>

#include <antwika/character/Character.hpp>
#include <antwika/map/MapFile.hpp>
#include <antwika/map/MapFileError.hpp>
#include "MapFileShared.hpp"

namespace antwika::map
{
    using namespace mapfile;

    namespace
    {
        void mapV1ToV2(nlohmann::json &document)
        {
            document[std::string(kRulesKey)] =
                nlohmann::json::array();
        }

        void mapV2ToV3(nlohmann::json &document)
        {
            nlohmann::json tileset;

            tileset[std::string(kPaletteKey)] =
                nlohmann::json::array();
            tileset[std::string(kSheetsKey)] =
                nlohmann::json::array();

            document[std::string(kTilesetKey)] = tileset;
        }

        void mapV18ToV19(nlohmann::json &)
        {
        }

        void mapV17ToV18(nlohmann::json &document)
        {
            nlohmann::json base;

            base[std::string(kNameKey)] =
                std::string(kBaseLayerName);
            document[std::string(kLayersKey)] =
                nlohmann::json::array({base});
        }

        void mapV16ToV17(nlohmann::json &document)
        {
            document[std::string(kLampsKey)] =
                nlohmann::json::array();
        }

        void mapV15ToV16(nlohmann::json &)
        {
        }

        void mapV14ToV15(nlohmann::json &document)
        {
            auto &camera = document[std::string(kCameraKey)];

            if (!camera.is_object())
            {
                return;
            }

            const auto zoom =
                camera[std::string(kZoomKey)].get<std::int64_t>();
            const auto scaledZoom =
                static_cast<double>(zoom)
                / static_cast<double>(kCameraScale);
            const auto halfHeight =
                scaledZoom > 0.0
                    ? static_cast<double>(camera::kCanvasSize.height)
                          / (2.0 * scaledZoom)
                    : static_cast<double>(camera::kDefaultZoom);
            const auto scaledCount = static_cast<std::int64_t>(
                std::llround(
                    halfHeight / static_cast<double>(camera::kVoxelPixels)));
            const auto steps = std::max(std::int64_t{1}, scaledCount);

            camera[std::string(kZoomKey)] = std::clamp(
                static_cast<std::int64_t>(camera::kVoxelPixels) * steps,
                static_cast<std::int64_t>(camera::kMinZoom),
                static_cast<std::int64_t>(camera::kMaxZoom));
        }

        void mapV13ToV14(nlohmann::json &document)
        {
            document[std::string(kTileLevelsKey)] =
                nlohmann::json::array();
        }

        void mapV12ToV13(nlohmann::json &document)
        {
            document[std::string(kTileFacingsKey)] =
                nlohmann::json::array();
        }

        void mapV11ToV12(nlohmann::json &document)
        {
            document[std::string(kTileKindsKey)] =
                nlohmann::json::array();
        }
        void mapV21ToV22(nlohmann::json &document)
        {
            document[std::string(kDecorKey)] =
                nlohmann::json::array();
            document[std::string(kDecorRulesKey)] =
                nlohmann::json::array();
            document[std::string(kStartKey)] = nlohmann::json();
            document[std::string(kExitKey)] = nlohmann::json();
        }
    }

    namespace mapfile
    {
        void earlyMapMigrations(
            schema::MigrationList &migrations)
        {
            migrations.push_back(schema::getMigration(
                1,
                2,
                "antwika::map: a map kept no rules before",
                mapV1ToV2));
            migrations.push_back(schema::getMigration(
                2,
                3,
                "antwika::map: a map carried no tileset "
                "before",
                mapV2ToV3));
            migrations.push_back(schema::getMigration(
                18,
                19,
                "antwika::map: a palette may now be added "
                "to and taken from",
                mapV18ToV19));
            migrations.push_back(schema::getMigration(
                17,
                18,
                "antwika::map: a map is now drawn in layers",
                mapV17ToV18));
            migrations.push_back(schema::getMigration(
                16,
                17,
                "antwika::map: lamps may now be set down "
                "about the pile",
                mapV16ToV17));
            migrations.push_back(schema::getMigration(
                15,
                16,
                "antwika::map: a ramp may now be told which "
                "way it climbs",
                mapV15ToV16));
            migrations.push_back(schema::getMigration(
                14,
                15,
                "antwika::map: the camera now keeps how many "
                "pixels a voxel is drawn across, whole",
                mapV14ToV15));
            migrations.push_back(schema::getMigration(
                13,
                14,
                "antwika::map: a tile now says which level "
                "of a flight of steps it was drawn for",
                mapV13ToV14));
            migrations.push_back(schema::getMigration(
                12,
                13,
                "antwika::map: a tile now says which way "
                "the stair it was drawn for climbs",
                mapV12ToV13));
            migrations.push_back(schema::getMigration(
                11,
                12,
                "antwika::map: a map now says which tiles "
                "belong to which kind of voxel",
                mapV11ToV12));
            migrations.push_back(schema::getMigration(
                21,
                22,
                "antwika::map: a map now keeps its decor, "
                "and where the character starts and leaves",
                mapV21ToV22));
        }
    }

}
