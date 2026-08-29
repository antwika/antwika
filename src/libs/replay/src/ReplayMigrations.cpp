#include "antwika/replay/ReplayMigrations.hpp"

#include <array>
#include <utility>

#include <antwika/schema/MigrationRow.hpp>
#include <antwika/replay/ReplayVersions.hpp>

namespace antwika::replay
{

    using schema::MigrationList;

    namespace
    {
        void recordV1ToV2(nlohmann::json &)
        {
        }
    }

    MigrationChain getStandardReplayMigrations()
    {
        MigrationList migrations;
        const std::array rows{
            schema::MigrationRow{
                1,
                2,
                "replay: a record is unchanged by JSON Lines",
                recordV1ToV2}};

        schema::pushMigrations(migrations, rows);

        return MigrationChain(
            std::move(migrations), kReplayDocumentVersion);
    }

}
