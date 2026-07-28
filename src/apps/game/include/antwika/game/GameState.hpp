#pragma once

#include <cstdint>

namespace antwika::game
{

    // Deliberately plain data.
    // The engine core has no opinion about what "score" means.
    // So this state is owned and defined entirely in the application.
    // It's not defined in any engine library.
    struct GameState
    {
        std::uint64_t ticksProcessed{};
        std::uint64_t score{};
        bool operator==(const GameState &other) const = default;
    };

} // namespace antwika::game
