#pragma once

#include <antwika/replay/MigrationChain.hpp>

namespace antwika::replay
{

    /**
     * @brief Build the migration chain for the replay document format.
     * @return A chain that brings a replay document of any version this
     * build still reads up to kReplayDocumentVersion.
     *
     * The chain is empty today, because the format is still at version
     * 1 and there is nothing to migrate from.
     * It is a factory rather than a constant so that adding the first
     * migration changes one function and nothing else, and so that
     * nobody is tempted to make it a global.
     *
     * A save file, or any other versioned document, writes its own
     * factory beside this one; see MigrationChain for the shape of it.
     */
    [[nodiscard]] MigrationChain standardReplayMigrations();

} // namespace antwika::replay
