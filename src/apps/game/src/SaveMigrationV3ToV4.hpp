#pragma once

#include <cstdint>
#include <string_view>

#include <nlohmann/json.hpp>

#include <antwika/replay/IMigration.hpp>

namespace antwika::game
{

    /**
     * @brief Version 3 counted coverage for four services.
     *
     * Safety and Structure left the service list when the risks
     * stopped answering to coverage: a fire risk and a collapse risk
     * climb on their own now, and a fireman or an engineer knocks one
     * back down by visiting rather than by refreshing a countdown.
     *
     * A version 3 coverage array is therefore two entries too long,
     * and this truncates it to the two services that remain -- water
     * and health, which kept the slots they had.
     * The dropped countdowns name nothing any build still reads, so
     * dropping them is the honest reading rather than a repair.
     *
     * A building without a coverage array is left exactly as it is,
     * since absent already means "nothing has ever reached it".
     */
    class DropRiskServices final : public antwika::replay::IMigration
    {
    public:
        /**
         * @brief The version this reads.
         * @return Three.
         */
        [[nodiscard]] std::uint32_t fromVersion() const noexcept override;

        /**
         * @brief The version this produces.
         * @return Four.
         */
        [[nodiscard]] std::uint32_t toVersion() const noexcept override;

        /**
         * @brief What this step is called, for a diagnostic.
         * @return Its name.
         */
        [[nodiscard]] std::string_view name() const noexcept override;

        /**
         * @brief Bring one document from version 3 to version 4.
         * @param document The document to rewrite in place.
         */
        void apply(nlohmann::json &document) const override;
    };

} // namespace antwika::game
