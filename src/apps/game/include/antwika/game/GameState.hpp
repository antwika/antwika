#pragma once

#include <cstdint>

namespace antwika::game
{

    inline constexpr std::int64_t kStartingMoney = 5000;

    struct GameState final
    {
        std::uint64_t ticksProcessed{};

        std::uint64_t score{};

        std::int64_t money{kStartingMoney};

        bool operator==(const GameState &other) const = default;
    };

}
