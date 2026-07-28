#pragma once

#include "ILogPolicy.hpp"

namespace antwika::log
{

    /**
     * @brief ILogPolicy that accepts levels at or above a configured threshold.
     */
    class MinimumLevelLogPolicy final : public ILogPolicy
    {
    public:
        /**
         * @brief Construct the policy with its acceptance threshold.
         * @param minimumLevel The lowest level that will be accepted.
         */
        explicit MinimumLevelLogPolicy(Level minimumLevel);

        MinimumLevelLogPolicy(const MinimumLevelLogPolicy &) = delete;
        MinimumLevelLogPolicy(MinimumLevelLogPolicy &&) = delete;

        MinimumLevelLogPolicy &operator=(
            const MinimumLevelLogPolicy &) = delete;
        MinimumLevelLogPolicy &operator=(MinimumLevelLogPolicy &&) = delete;

        /**
         * @brief Check whether a level meets the configured threshold.
         * @param level The severity level to check.
         * @return true if level is at or above the minimum, false otherwise.
         */
        [[nodiscard]] bool accepts(Level level) const noexcept override;

    private:
        Level minimumLevel;
    };

} // namespace antwika::log
