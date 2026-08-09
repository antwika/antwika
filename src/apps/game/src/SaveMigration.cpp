#include <nlohmann/json.hpp>

#include <memory>
#include <string_view>

#include <antwika/replay/IMigration.hpp>
#include <antwika/replay/MigrationChain.hpp>

#include "antwika/game/SaveGame.hpp"
#include "antwika/game/Walker.hpp"
#include "RenameToServices.hpp"
#include "DropRiskServices.hpp"

namespace antwika::game
{

    namespace
    {
        class AddBuildings final : public antwika::replay::IMigration
        {
        public:
            [[nodiscard]] std::uint32_t fromVersion() const noexcept override
            {
                return 1;
            }

            [[nodiscard]] std::uint32_t toVersion() const noexcept override
            {
                return 2;
            }

            // GCOVR_EXCL_START
            [[nodiscard]] std::string_view name() const noexcept override
            {
                return "add buildings";
            }
            // GCOVR_EXCL_STOP

            void apply(nlohmann::json &document) const override
            {
                document["buildings"] = nlohmann::json::array();

                for (auto &walker : document["walkers"])
                {
                    walker["kind"] = "food";
                    walker["carried"] = 0;
                    walker["stepsUntilHome"] = kRoamingSteps;
                    walker["ticksUntilStep"] = 0;
                }
            }
        };
    }

    MigrationChain standardSaveMigrations()
    {
        // GCOVR_EXCL_START
        return MigrationChain(
            {std::make_shared<const AddBuildings>(),
             std::make_shared<const RenameToServices>(),
             std::make_shared<const DropRiskServices>()},
            kSaveFormatVersion);
        // GCOVR_EXCL_STOP
    }

}
