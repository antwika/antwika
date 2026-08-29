#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>

#include <antwika/schema/MigrationChain.hpp>
#include <antwika/schema/Step.hpp>

#include <antwika/map/MapFile.hpp>

#include "MapFileShared.hpp"

namespace antwika::map
{
    using namespace mapfile;

    namespace
    {
        void mapV22ToV23(nlohmann::json &document)
        {
            constexpr int kEveryBase = 100;

            for (auto &decor :
                 document[std::string(kDecorKey)])
            {
                decor[std::string(kFrequencyKey)] =
                    kEveryBase;
            }
        }

        void mapV23ToV24(nlohmann::json &document)
        {
            for (auto &decor :
                 document[std::string(kDecorKey)])
            {
                decor[std::string(kNameKey)] = "";
            }
        }

        void mapV24ToV25(nlohmann::json &document)
        {
            auto &settings =
                document[std::string(kSettingsKey)];

            settings[std::string(kGridKey)] = true;
            settings[std::string(kMarkerKey)] = true;
            settings[std::string(kSightKey)] = true;
            settings[std::string(kFollowingKey)] = true;
            settings[std::string(kAboveHiddenKey)] = false;
            settings[std::string(kCornersJoinedKey)] = false;
        }

        void mapV25ToV26(nlohmann::json &document)
        {
            auto glows = nlohmann::json::array();

            for (std::size_t index = 0;
                 index < document[std::string(kPaletteKey)]
                          .size();
                 ++index)
            {
                glows.push_back(0);
            }

            document[std::string(kGlowsKey)] = glows;
            document[std::string(kAmbientKey)] = 0;
        }

        void mapV26ToV27(nlohmann::json &document)
        {
            (void)document;
        }

        void mapV27ToV28(nlohmann::json &document)
        {
            (void)document;
        }

        void mapV28ToV29(nlohmann::json &document)
        {
            document[std::string(kExitTargetKey)] = "";
        }

    }

    namespace mapfile
    {
        void mapMigrationsV22To30(
            schema::MigrationList &migrations)
        {
            const std::array rows{
                MigrationRow{
                    .fromVersion = 22,
                    .toVersion = 23,
                    .name = "antwika::map: a decor now says how often "
                    "it takes a base it may stand on",
                    .apply = mapV22ToV23},
                MigrationRow{
                    .fromVersion = 23,
                    .toVersion = 24,
                    .name = "antwika::map: a decor now carries a name "
                    "the artist may give it",
                    .apply = mapV23ToV24},
                MigrationRow{
                    .fromVersion = 24,
                    .toVersion = 25,
                    .name = "antwika::map: a map now keeps the view "
                    "toggles as they were left",
                    .apply = mapV24ToV25},
                MigrationRow{
                    .fromVersion = 25,
                    .toVersion = 26,
                    .name = "antwika::map: a map now says how its inks "
                    "glow and how much light the world keeps",
                    .apply = mapV25ToV26},
                MigrationRow{
                    .fromVersion = 26,
                    .toVersion = 27,
                    .name = "antwika::map: a stamp tool joined the "
                    "panel",
                    .apply = mapV26ToV27},
                MigrationRow{
                    .fromVersion = 27,
                    .toVersion = 28,
                    .name = "antwika::map: cubes may be ladders now",
                    .apply = mapV27ToV28},
                MigrationRow{
                    .fromVersion = 28,
                    .toVersion = 29,
                    .name = "antwika::map: an exit may lead to "
                    "another map now",
                    .apply = mapV28ToV29},
                MigrationRow{
                    .fromVersion = 29,
                    .toVersion = 30,
                    .name = "antwika::map: a map may hold figures "
                    "and plates now",
                    .apply = createEmptyArrays({kFiguresKey, kPlatesKey})}};

            pushMigrations(migrations, rows);
        }
    }

}
