#include "SaveMigration.hpp"

#include <string>

#include "antwika/game/SaveFormatError.hpp"
#include "antwika/game/SaveGame.hpp"

namespace antwika::game::detail
{

    namespace
    {
        constexpr const char *kVersionMember = "schemaVersion";
    } // namespace

    std::uint32_t saveVersionOf(const nlohmann::json &j)
    {
        // Asked before the schema runs.
        // So this is the one place that has to say "not an object".
        if (!j.is_object())
        {
            throw SaveFormatError(
                "antwika::game: a save document must be a JSON object");
        }

        const auto version = j.find(kVersionMember);
        if (version == j.end())
        {
            return 1;
        }

        if (!version->is_number_unsigned())
        {
            throw SaveFormatError(
                "antwika::game: a save's schemaVersion must be a "
                "non-negative integer");
        }

        return version->get<std::uint32_t>();
    }

    void migrateSaveDocument(nlohmann::json &document, std::uint32_t from)
    {
        // See the header: this body is the seam.
        // replay's migration chain replaces it whole.
        // The cast marks the argument used while one version exists.
        (void)document;

        if (from != kSaveFormatVersion)
        {
            throw SaveFormatError(
                "antwika::game: a save of schema version "
                + std::to_string(from)
                + " cannot be read by this build, which reads version "
                + std::to_string(kSaveFormatVersion));
        }
    }

} // namespace antwika::game::detail
