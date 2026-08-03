#pragma once

#include <cstdint>

namespace antwika::game
{

    /**
     * @brief What a session opens with in the bank.
     *
     * A default member value rather than a rule anywhere, so a new
     * game, a fresh GameState in a test and a save written before
     * money existed all start from the same number by construction.
     */
    inline constexpr std::int64_t kStartingMoney = 5000;

    /**
     * @brief Deliberately plain data describing this application's state.
     *
     * The engine core has no opinion about what "score" means, so this
     * state is owned and defined entirely in the application, not in any
     * engine library.
     */
    struct GameState
    {
        /// Number of engine ticks folded into this state so far.
        std::uint64_t ticksProcessed{};

        /// Running total accumulated from kScoreIncrement events.
        std::uint64_t score{};

        /**
         * @brief What is left in the bank.
         *
         * Spent by GridSink as roads and buildings are placed, and by
         * nothing else; costOf() and kRoadCost say what each placement
         * takes. **Spending is never refused**, so the balance may go
         * negative: nothing pays money in yet, and a placement refused
         * at zero would end every session for good the moment the bank
         * ran out. Signed for exactly that reason.
         *
         * Simulation state on the camera's terms -- every change to it
         * is derived from a recorded click, so a replay regenerates the
         * balance and GameSummary compares it.
         */
        std::int64_t money{kStartingMoney};

        bool operator==(const GameState &other) const = default;
    };

} // namespace antwika::game
