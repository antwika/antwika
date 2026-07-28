#pragma once

#include <cstdint>

namespace antwika::game
{

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

        bool operator==(const GameState &other) const = default;
    };

} // namespace antwika::game
