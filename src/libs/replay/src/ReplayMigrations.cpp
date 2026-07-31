#include "antwika/replay/ReplayMigrations.hpp"

#include <antwika/replay/SchemaVersion.hpp>

namespace antwika::replay
{

    MigrationChain standardReplayMigrations()
    {
        // Nothing to migrate yet: the format is at version 1.
        // Bumping kReplayDocumentVersion adds one migration here.
        return MigrationChain({}, kReplayDocumentVersion);
    }

} // namespace antwika::replay
