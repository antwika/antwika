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
    }

    MigrationChain standardReplayMigrations()
    {
        MigrationList migrations;
        migrations.push_back(std::make_shared<const RecordV1ToV2>());

        return MigrationChain(
            std::move(migrations), kReplayDocumentVersion);
    }

}
