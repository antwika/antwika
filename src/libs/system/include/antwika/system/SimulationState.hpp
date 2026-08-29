#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

namespace antwika::system
{

    struct SimulationState final
    {
        bool walkerHeld = false;

        bool simulationPaused = false;

        bool running = false;

        std::size_t characterCount = 0;

        std::optional<std::uint32_t> speaking = std::nullopt;

        [[nodiscard]] bool operator==(
            const SimulationState &other) const = default;
    };

}
