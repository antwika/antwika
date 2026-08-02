#include "antwika/replay/ReplayMigrations.hpp"

#include <cstdint>
#include <memory>
#include <string_view>
#include <utility>

#include <antwika/replay/IMigration.hpp>
#include <antwika/replay/SchemaVersion.hpp>

namespace antwika::replay
{

    namespace
    {
        // Version 2 is JSON Lines.
        // Version 1 was one JSON object holding an "events" array.
        // What changed is how records are framed in a file.
        // Not what a record is.
        // It is {"tick": N, "event": {...}} in both.
        //
        // So there is nothing here to rewrite.
        // The step exists all the same, because a chain refuses a gap.
        // Reaching version 2 from version 1 is a step somebody wrote.
        // Which beats a version nobody accounted for.
        class RecordV1ToV2 final : public IMigration
        {
        public:
            [[nodiscard]] std::uint32_t fromVersion() const noexcept
                override
            {
                return 1;
            }

            [[nodiscard]] std::uint32_t toVersion() const noexcept
                override
            {
                return 2;
            }

            // MigrationChain asks for this in one place only.
            // It is the message thrown when a step is not one step.
            // This one reads 1 and produces 2.
            // So reaching it means editing the two functions above.
            // Which breaks the migration rather than feeding it input.
            // See docs/confirming-unreachable-branches.md.
            // GCOVR_EXCL_START
            [[nodiscard]] std::string_view name() const noexcept override
            {
                return "replay: a record is unchanged by JSON Lines";
            }
            // GCOVR_EXCL_STOP

            void apply(nlohmann::json &) const override
            {
            }
        };
    } // namespace

    MigrationChain standardReplayMigrations()
    {
        // The version key is the shared one, so none is passed.
        // A record states no version of its own all the same.
        // What the chain is handed is the version the header stated.
        // Through MigrationChain::migrateFrom().
        MigrationList migrations;
        migrations.push_back(std::make_shared<const RecordV1ToV2>());

        return MigrationChain(
            std::move(migrations), kReplayDocumentVersion);
    }

} // namespace antwika::replay
