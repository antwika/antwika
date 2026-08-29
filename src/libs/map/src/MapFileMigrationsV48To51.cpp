#include <nlohmann/json.hpp>

#include <array>
#include <string>

#include <antwika/schema/MigrationChain.hpp>

#include <antwika/map/MapFile.hpp>

#include "MapFileShared.hpp"
#include "MapFileShared2.hpp"

namespace antwika::map
{
    using namespace mapfile;

    namespace
    {
        constexpr std::string_view kOldIndexName =
            "component::RosterIndex";

        constexpr std::string_view kNewIndexName =
            "component::CharacterIndex";

        void mapV48ToV49(nlohmann::json &document)
        {
            document.erase(std::string(kKeysKey));
            document.erase(std::string(kDoorsKey));
            document.erase(std::string(kPlatesKey));
            document.erase(std::string(kExitLockedKey));
        }

        void mapV49ToV50(nlohmann::json &document)
        {
            for (auto &character :
                 document[std::string(kCharactersKey)])
            {
                character[std::string(kComponentValuesKey)] =
                    character[std::string(kTuningKey)];
                character.erase(std::string(kTuningKey));
            }
        }

        void mapV50ToV51(nlohmann::json &document)
        {
            for (auto &character :
                 document[std::string(kCharactersKey)])
            {
                for (auto &name :
                     character[std::string(kComponentsKey)])
                {
                    if (name.get<std::string>() == kOldIndexName)
                    {
                        name = std::string(kNewIndexName);
                    }
                }

                auto &values =
                    character[std::string(kComponentValuesKey)];

                if (values.contains(std::string(kOldIndexName)))
                {
                    values[std::string(kNewIndexName)] =
                        values[std::string(kOldIndexName)];
                    values.erase(std::string(kOldIndexName));
                }
            }
        }
    }

    namespace mapfile
    {
        void mapMigrationsV48To51(
            schema::MigrationList &migrations)
        {
            const std::array rows{
                MigrationRow{
                    .fromVersion = 48,
                    .toVersion = 49,
                    .name = "antwika::map: the keys, doors, pressure "
                    "plates and the locked exit leave the map until "
                    "they are redesigned",
                    .apply = mapV48ToV49},
                MigrationRow{
                    .fromVersion = 49,
                    .toVersion = 50,
                    .name = "antwika::map: a character's tuning is "
                    "named componentValues, for the values it sets "
                    "on the components it carries",
                    .apply = mapV49ToV50},
                MigrationRow{
                    .fromVersion = 50,
                    .toVersion = 51,
                    .name = "antwika::map: the roster index is named "
                    "the character index",
                    .apply = mapV50ToV51}};

            pushMigrations(migrations, rows);
        }
    }

}
