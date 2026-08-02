#pragma once

#include <cstdint>
#include <string_view>

#include <nlohmann/json.hpp>

#include <antwika/replay/IMigration.hpp>

namespace antwika::game
{

    /**
     * @brief Version 2 spoke of food and water, and of one walker out.
     *
     * Three things changed under it at once, and all three are breaking
     * rather than additive -- which is why this increment bumps the
     * version exactly once and this is the only migration it writes.
     *
     * **A building's walker became a list.** A building may now have
     * kMaxWalkersOut out at a time, so the one index it wrote is wrapped
     * in an array of one; a file that named nobody still names nobody.
     *
     * **Water stopped being a good.** A well confers coverage on what
     * its carrier walks past rather than handing an amount over, so the
     * resources became food, clay and pottery, and a stock of
     * `[food, water]` reads as `[food, 0, 0]`: the food is what it was,
     * the water was never a good to begin with, and nobody has carted a
     * cartload of anything to that building yet.
     *
     * **The vocabulary was renamed.** A food source is a farm, a water
     * source is a well and an architect's post is an engineer's, and the
     * walkers they send are named for what they do rather than for what
     * they used to carry.
     *
     * A name this migration does not recognise is left exactly as it is
     * rather than guessed at, so the decode is what refuses it and the
     * message names the word it did not know.
     */
    class RenameToServices final : public antwika::replay::IMigration
    {
    public:
        /**
         * @brief The version this reads.
         * @return Two.
         */
        [[nodiscard]] std::uint32_t fromVersion() const noexcept override;

        /**
         * @brief The version this produces.
         * @return Three.
         */
        [[nodiscard]] std::uint32_t toVersion() const noexcept override;

        /**
         * @brief What this step is called, for a diagnostic.
         * @return Its name.
         */
        [[nodiscard]] std::string_view name() const noexcept override;

        /**
         * @brief Bring one document from version 2 to version 3.
         * @param document The document to rewrite in place.
         */
        void apply(nlohmann::json &document) const override;
    };

} // namespace antwika::game
