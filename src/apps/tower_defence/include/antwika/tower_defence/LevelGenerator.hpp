#pragma once

#include <cstdint>

#include "antwika/tower_defence/Level.hpp"

namespace antwika::tower_defence
{

    struct LevelConfig final
    {
        std::uint32_t width = 12;

        std::uint32_t height = 8;

        std::uint64_t seed = 0;

        std::uint32_t wallSpacing = 3;

        std::uint64_t initialSolverSteps = 200;

        std::uint64_t maxSolverSteps = 20000;

        std::uint32_t maxAttempts = 64;
    };

    [[nodiscard]] Level generateLevel(const LevelConfig &config);

}
