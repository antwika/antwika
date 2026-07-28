#pragma once

#include <cstdint>

namespace antwika::game
{

    // Deliberately plain data: the engine core has no opinion about what "score" means, so state is owned and defined entirely here, in the application, not in any engine library.
    struct GameState
    {
        std::uint64_t ticksProcessed{};
        std::uint64_t score{};
        bool operator==(const GameState &other) const = default;
    };

} // namespace antwika::game
