#include <nlohmann/json.hpp>

#include <array>
#include <string>

#include <antwika/schema/MigrationChain.hpp>

#include <antwika/map/MapFile.hpp>

#include "MapFileShared2.hpp"

namespace antwika::map
{
    using namespace mapfile;

    namespace
    {
        void mapV47ToV48(nlohmann::json &document)
        {
            for (auto &figure :
                 document[std::string(kCharactersKey)])
            {
                figure[std::string(kTuningKey)] =
                    nlohmann::json::object();
            }
        }
    }

    namespace mapfile
    {
        void mapMigrationsV47To48(
            schema::MigrationList &migrations)
        {
            const std::array rows{
                MigrationRow{
                    .fromVersion = 47,
                    .toVersion = 48,
                    .name = "antwika::map: a figure may tune the "
                    "components it carries away from their "
                    "fresh values now",
                    .apply = mapV47ToV48}};

            pushMigrations(migrations, rows);
        }
    }

}
