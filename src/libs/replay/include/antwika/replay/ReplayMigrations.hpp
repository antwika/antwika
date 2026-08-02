#pragma once

#include <antwika/replay/MigrationChain.hpp>

namespace antwika::replay
{

    /**
     * @brief Build the migration chain for a replay's records.
     * @return A chain that brings one record of any version this build
     * still reads up to kReplayDocumentVersion.
     *
     * **The unit is a record, not a file.** A replay is an event log,
     * and the log is what a version describes the shape of; the header
     * that states the version is never migrated, since it has to stay
     * legible to every build there will ever be.
     * So this chain is applied once a line rather than once a file,
     * through MigrationChain::migrateFrom(), at the version the header
     * stated.
     *
     * It is a factory rather than a constant so that adding a migration
     * changes one function and nothing else, and so that nobody is
     * tempted to make it a global.
     *
     * A save file, or any other versioned document, writes its own
     * factory beside this one; see MigrationChain for the shape of it.
     */
    [[nodiscard]] MigrationChain standardReplayMigrations();

} // namespace antwika::replay
