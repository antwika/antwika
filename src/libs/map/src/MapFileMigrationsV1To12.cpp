#include <nlohmann/json.hpp>

#include <array>
#include <cmath>
#include <cstddef>

#include <antwika/schema/MigrationChain.hpp>
#include <antwika/schema/Step.hpp>

#include <antwika/map/MapFile.hpp>

#include "MapFileShared.hpp"

namespace antwika::map
{
    using namespace mapfile;

    namespace
    {
        void mapV2ToV3(nlohmann::json &document)
        {
            nlohmann::json tileset;

            tileset[std::string(kPaletteKey)] =
                nlohmann::json::array();
            tileset[std::string(kSheetsKey)] =
                nlohmann::json::array();

            document[std::string(kTilesetKey)] = tileset;
        }

        void mapV3ToV4(nlohmann::json &document)
        {
            constexpr std::array<std::array<int, 4>, 6> kFirstPalette{
                {{0, 39, 43, 255},
                 {49, 56, 28, 255},
                 {3, 82, 63, 255},
                 {133, 98, 65, 255},
                 {159, 166, 108, 255},
                 {217, 211, 152, 255}}};

            const auto palette =
                document[std::string(kTilesetKey)]
                        [std::string(kPaletteKey)];
            auto colors = nlohmann::json::array();

            for (std::size_t index = 0;
                 index < kFirstPalette.size();
                 ++index)
            {
                colors.push_back(
                    index < palette.size()
                        ? palette[index]
                        : nlohmann::json::array(
                              {kFirstPalette.at(index).at(0),
                               kFirstPalette.at(index).at(1),
                               kFirstPalette.at(index).at(2),
                               kFirstPalette.at(index).at(3)}));
            }

            document[std::string(kPaletteKey)] = colors;
            document.erase(std::string(kTilesetKey));
        }

        void mapV4ToV5(nlohmann::json &)
        {
        }

        void mapV5ToV6(nlohmann::json &document)
        {
            for (auto &rule : document[std::string(kRulesKey)])
            {
                rule[std::string(kAirKey)] = true;
            }
        }

        void mapV7ToV8(nlohmann::json &document)
        {
            document[std::string(kCameraKey)] = nlohmann::json();
        }

        void mapV8ToV9(nlohmann::json &document)
        {
            auto &camera = document[std::string(kCameraKey)];

            if (!camera.is_null())
            {
                constexpr double kFirstZoom = 15.0;

                camera[std::string(kZoomKey)] =
                    std::llround(kFirstZoom * kCameraScale);
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

    }

    namespace mapfile
    {
        void mapMigrationsV1To12(
            schema::MigrationList &migrations)
        {
            const std::array rows{
                MigrationRow{
                    .fromVersion = 1,
                    .toVersion = 2,
                    .name = "antwika::map: a map kept no rules before",
                    .apply = createEmptyArrays({kRulesKey})},
                MigrationRow{
                    .fromVersion = 2,
                    .toVersion = 3,
                    .name = "antwika::map: a map carried no tileset "
                    "before",
                    .apply = mapV2ToV3},
                MigrationRow{
                    .fromVersion = 3,
                    .toVersion = 4,
                    .name = "antwika::map: a map carries a palette, the "
                    "pixels living in the atlas files",
                    .apply = mapV3ToV4},
                MigrationRow{
                    .fromVersion = 4,
                    .toVersion = 5,
                    .name = "antwika::map: an edge may now be shut "
                    "against everything",
                    .apply = mapV4ToV5},
                MigrationRow{
                    .fromVersion = 5,
                    .toVersion = 6,
                    .name = "antwika::map: an edge now says whether it "
                    "may lie at the rim",
                    .apply = mapV5ToV6},
                MigrationRow{
                    .fromVersion = 6,
                    .toVersion = 7,
                    .name = "antwika::map: a tile may now ask what "
                    "stands beyond its corners",
                    .apply = createEmptyArrays({kCornersKey})},
                MigrationRow{
                    .fromVersion = 7,
                    .toVersion = 8,
                    .name = "antwika::map: a map now says where the "
                    "camera stood",
                    .apply = mapV7ToV8},
                MigrationRow{
                    .fromVersion = 8,
                    .toVersion = 9,
                    .name = "antwika::map: a map now says how much the "
                    "camera showed",
                    .apply = mapV8ToV9},
                MigrationRow{
                    .fromVersion = 9,
                    .toVersion = 10,
                    .name = "antwika::map: a map now says how the "
                    "editor was left standing",
                    .apply = mapV9ToV10},
                MigrationRow{
                    .fromVersion = 10,
                    .toVersion = 11,
                    .name = "antwika::map: a voxel now says what it is "
                    "made of",
                    .apply = mapV10ToV11},
                MigrationRow{
                    .fromVersion = 11,
                    .toVersion = 12,
                    .name = "antwika::map: a map now says which tiles "
                    "belong to which kind of voxel",
                    .apply = createEmptyArrays({kTileKindsKey})}};

            pushMigrations(migrations, rows);
        }

    }

}
