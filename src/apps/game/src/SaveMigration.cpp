#include "SaveMigration.hpp"

#include <antwika/replay/MigrationChain.hpp>
#include <antwika/replay/SchemaVersionError.hpp>

#include "antwika/game/SaveFormatError.hpp"
#include "antwika/game/SaveGame.hpp"

namespace antwika::game
{

    MigrationChain standardSaveMigrations()
    {
        // Nothing to migrate yet: the format is at version 1.
        // Bumping kSaveFormatVersion adds one migration here.
        // The version key is the shared one, so none is passed.
        return MigrationChain({}, kSaveFormatVersion);
    }

    namespace detail
    {

        void migrateSaveDocument(nlohmann::json &document)
        {
            try
            {
                standardSaveMigrations().migrate(document);
            }
            // GCOVR_EXCL_START
            catch (const replay::SchemaVersionError &error)
            {
                // Translated rather than let through.
                // A bad save is this app's failure category.
                // saveGameFromJson() promises one exception type.
                // The chain's message names both versions already.
                // So it is carried through rather than rewritten.
                throw SaveFormatError(error.what());
            }
            // GCOVR_EXCL_STOP
        }

    } // namespace detail

} // namespace antwika::game
