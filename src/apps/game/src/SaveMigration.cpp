#include <memory>
#include <string_view>

#include <nlohmann/json.hpp>

#include <antwika/replay/IMigration.hpp>
#include <antwika/replay/MigrationChain.hpp>

#include "antwika/game/SaveGame.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    namespace
    {
        /**
         * @brief Version 1 knew nothing about buildings.
         *
         * A version 1 file recorded the roads and the walkers and
         * nothing standing on the grid, so the honest reading of one is
         * a city with no buildings in it.
         * An empty array is exactly that, and it is all this has to add:
         * a walker in such a file names no home, which is already an
         * ordinary state.
         */
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

            // MigrationChain asks for this in one place only.
            // It is the message thrown when a migration is not one step.
            // This one reads 1 and produces 2.
            // So reaching it means editing the two functions above.
            // Which breaks the migration rather than feeding it input.
            // See docs/confirming-unreachable-branches.md.
            // GCOVR_EXCL_START
            [[nodiscard]] std::string_view name() const noexcept override
            {
                return "add buildings";
            }
            // GCOVR_EXCL_STOP

            void apply(nlohmann::json &document) const override
            {
                document["buildings"] = nlohmann::json::array();

                // Version 1 walkers carried none of the economy.
                // So each is filled in as one that has just set out.
                for (auto &walker : document["walkers"])
                {
                    walker["kind"] = "food";
                    walker["carried"] = 0;
                    walker["stepsUntilHome"] = kRoamingSteps;
                    walker["ticksUntilStep"] = 0;
                }
            }
        };
    } // namespace

    MigrationChain standardSaveMigrations()
    {
        // The version key is the shared one, so none is passed.
        // The branches left on the excluded line are the allocator's:
        // the throw edge of the make_shared and of the list it goes in.
        // Nothing here is large enough to take the heap branch.
        // Confirmed with gcov -b, as the coverage doc requires.
        // GCOVR_EXCL_START
        return MigrationChain(
            {std::make_shared<const AddBuildings>()}, kSaveFormatVersion);
        // GCOVR_EXCL_STOP
    }

} // namespace antwika::game
