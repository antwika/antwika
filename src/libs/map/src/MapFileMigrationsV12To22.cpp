#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cmath>

#include <antwika/schema/MigrationChain.hpp>
#include <antwika/schema/Step.hpp>

#include <antwika/map/MapFile.hpp>

#include "MapFileShared.hpp"

namespace antwika::map
{
    using namespace mapfile;

    namespace
    {
        void mapV14ToV15(nlohmann::json &document)
        {
            auto &camera = document[std::string(kCameraKey)];

            if (!camera.is_object())
            {
                return;
            }

            constexpr double kCanvasHeight = 270.0;
            constexpr double kFallbackZoom = 15.0;
            constexpr std::int64_t kVoxelPixels = 15;
            constexpr std::int64_t kLeastZoom = 5;
            constexpr std::int64_t kGreatestZoom = 120;

            const auto zoom =
                camera[std::string(kZoomKey)].get<std::int64_t>();
            const auto scaledZoom =
                static_cast<double>(zoom)
                / static_cast<double>(kCameraScale);
            const auto halfHeight =
                scaledZoom > 0.0
                    ? kCanvasHeight / (2.0 * scaledZoom)
                    : kFallbackZoom;
            const auto scaledCount = static_cast<std::int64_t>(
                std::llround(
                    halfHeight / static_cast<double>(kVoxelPixels)));
            const auto steps = std::max(std::int64_t{1}, scaledCount);

            camera[std::string(kZoomKey)] = std::clamp(
                kVoxelPixels * steps, kLeastZoom, kGreatestZoom);
        }

        void mapV15ToV16(nlohmann::json &)
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

        void mapV18ToV19(nlohmann::json &)
        {
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
        void mapMigrationsV12To22(
            schema::MigrationList &migrations)
        {
            const std::array rows{
                MigrationRow{
                    .fromVersion = 12,
                    .toVersion = 13,
                    .name = "antwika::map: a tile now says which way "
                    "the stair it was drawn for climbs",
                    .apply = createEmptyArrays({kTileFacingsKey})},
                MigrationRow{
                    .fromVersion = 13,
                    .toVersion = 14,
                    .name = "antwika::map: a tile now says which level "
                    "of a flight of steps it was drawn for",
                    .apply = createEmptyArrays({kTileLevelsKey})},
                MigrationRow{
                    .fromVersion = 14,
                    .toVersion = 15,
                    .name = "antwika::map: the camera now keeps how many "
                    "pixels a voxel is drawn across, whole",
                    .apply = mapV14ToV15},
                MigrationRow{
                    .fromVersion = 15,
                    .toVersion = 16,
                    .name = "antwika::map: a ramp may now be told which "
                    "way it climbs",
                    .apply = mapV15ToV16},
                MigrationRow{
                    .fromVersion = 16,
                    .toVersion = 17,
                    .name = "antwika::map: lamps may now be set down "
                    "about the pile",
                    .apply = createEmptyArrays({kLampsKey})},
                MigrationRow{
                    .fromVersion = 17,
                    .toVersion = 18,
                    .name = "antwika::map: a map is now drawn in layers",
                    .apply = mapV17ToV18},
                MigrationRow{
                    .fromVersion = 18,
                    .toVersion = 19,
                    .name = "antwika::map: a palette may now be added "
                    "to and taken from",
                    .apply = mapV18ToV19},
                MigrationRow{
                    .fromVersion = 19,
                    .toVersion = 20,
                    .name = "antwika::map: a map now says where the "
                    "character starts",
                    .apply = mapV19ToV20},
                MigrationRow{
                    .fromVersion = 20,
                    .toVersion = 21,
                    .name = "antwika::map: a character now starts facing "
                    "the way it faced",
                    .apply = mapV20ToV21},
                MigrationRow{
                    .fromVersion = 21,
                    .toVersion = 22,
                    .name = "antwika::map: a map now keeps its decor, "
                    "and where the character starts and leaves",
                    .apply = mapV21ToV22}};

            pushMigrations(migrations, rows);
        }
    }

}
