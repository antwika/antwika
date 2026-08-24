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
#include "MapFileShared2.hpp"

namespace antwika::map
{
    using namespace mapfile;

    namespace
    {
        void mapV10ToV11(nlohmann::json &document)
        {
            const auto solid =
                std::string(kKindNames.getName(voxel::Kind::Normal));

            if (document.contains(std::string(kVoxelsKey))
                && document[std::string(kVoxelsKey)].is_array())
            {
                auto voxels = nlohmann::json::array();

                for (const auto &place :
                     document[std::string(kVoxelsKey)])
                {
                    nlohmann::json voxel;

                    voxel[std::string(kAtKey)] = place;
                    voxel[std::string(kKindKey)] = solid;

                    voxels.push_back(voxel);
                }

                document[std::string(kVoxelsKey)] = voxels;
            }

            if (document.contains(std::string(kSettingsKey))
                && document[std::string(kSettingsKey)]
                       .is_object())
            {
                document[std::string(kSettingsKey)]
                        [std::string(kKindKey)] = solid;
            }
        }

        void mapV9ToV10(nlohmann::json &document)
        {
            nlohmann::json settings;

            settings[std::string(kLightingKey)] = true;
            settings[std::string(kTiesKey)] = true;
            settings[std::string(kToolKey)] = "brush";
            settings[std::string(kDrawingKey)] = "brush";
            settings[std::string(kViewKey)] = "world";

            document[std::string(kSettingsKey)] = settings;
        }

        void mapV8ToV9(nlohmann::json &document)
        {
            auto &camera = document[std::string(kCameraKey)];

            if (!camera.is_null())
            {
                camera[std::string(kZoomKey)] =
                    std::llround(
                        static_cast<double>(camera::kDefaultZoom)
                        * kCameraScale);
            }
        }

        void mapV7ToV8(nlohmann::json &document)
        {
            document[std::string(kCameraKey)] = nlohmann::json();
        }

        void mapV6ToV7(nlohmann::json &document)
        {
            document[std::string(kCornersKey)] =
                nlohmann::json::array();
        }

        void mapV5ToV6(nlohmann::json &document)
        {
            for (auto &rule : document[std::string(kRulesKey)])
            {
                rule[std::string(kAirKey)] = true;
            }
        }

        void mapV4ToV5(nlohmann::json &)
        {
        }

        void mapV3ToV4(nlohmann::json &document)
        {
                    const auto palette =
                        document[std::string(kTilesetKey)]
                                [std::string(kPaletteKey)];
                    auto colors = nlohmann::json::array();

                    for (std::size_t index = 0;
                         index < tile::kPaletteSize;
                         ++index)
                    {
                        colors.push_back(
                            index < palette.size()
                                ? palette[index]
                                : nlohmann::json::array(
                                    {tile::kPaletteColors.at(index).red,
                                     tile::kPaletteColors.at(index).green,
                                     tile::kPaletteColors.at(index).blue,
                                     tile::kPaletteColors.at(index).alpha}));
                    }

            document[std::string(kPaletteKey)] = colors;
                    document.erase(std::string(kTilesetKey));
        }

        void mapV19ToV20(nlohmann::json &document)
        {
            document[std::string(kWalkerKey)] = nlohmann::json();
        }

        void mapV20ToV21(nlohmann::json &document)
        {
            auto &walker = document[std::string(kWalkerKey)];

            if (walker.is_object())
            {
                walker[std::string(kWayKey)] = 0;
            }
        }

        }

    namespace mapfile
    {
        void lateMapMigrations(
            schema::MigrationList &migrations)
        {
            migrations.push_back(schema::getMigration(
                10,
                11,
                "antwika::map: a voxel now says what it is "
                "made of",
                mapV10ToV11));
            migrations.push_back(schema::getMigration(
                9,
                10,
                "antwika::map: a map now says how the "
                "editor was left standing",
                mapV9ToV10));
            migrations.push_back(schema::getMigration(
                8,
                9,
                "antwika::map: a map now says how much the "
                "camera showed",
                mapV8ToV9));
            migrations.push_back(schema::getMigration(
                7,
                8,
                "antwika::map: a map now says where the "
                "camera stood",
                mapV7ToV8));
            migrations.push_back(schema::getMigration(
                6,
                7,
                "antwika::map: a tile may now ask what "
                "stands beyond its corners",
                mapV6ToV7));
            migrations.push_back(schema::getMigration(
                5,
                6,
                "antwika::map: an edge now says whether it "
                "may lie at the rim",
                mapV5ToV6));
            migrations.push_back(schema::getMigration(
                4,
                5,
                "antwika::map: an edge may now be shut "
                "against everything",
                mapV4ToV5));
            migrations.push_back(schema::getMigration(
                3,
                4,
                "antwika::map: a map carries a palette, the "
                "pixels living in the atlas files",
                mapV3ToV4));
            migrations.push_back(schema::getMigration(
                19,
                20,
                "antwika::map: a map now says where the "
                "character starts",
                mapV19ToV20));
            migrations.push_back(schema::getMigration(
                20,
                21,
                "antwika::map: a character now starts facing "
                "the way it faced",
                mapV20ToV21));
        }

    }

}
