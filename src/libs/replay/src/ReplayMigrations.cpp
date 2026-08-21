#include "antwika/replay/ReplayMigrations.hpp"

#include <utility>

#include <antwika/schema/Step.hpp>
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

    MigrationChain standardReplayMigrations()
    {
        MigrationList migrations;
        migrations.push_back(schema::step(
            1,
            2,
            "replay: a record is unchanged by JSON Lines",
            recordV1ToV2));

        return MigrationChain(
            std::move(migrations), kReplayDocumentVersion);
    }

}
