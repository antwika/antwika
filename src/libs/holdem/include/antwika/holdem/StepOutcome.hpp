#pragma once

#include <cstdint>

#include "antwika/holdem/Action.hpp"
#include "antwika/holdem/Chips.hpp"
#include "antwika/holdem/SeatId.hpp"
#include "antwika/holdem/Stage.hpp"

namespace antwika::holdem
{

    enum class StepKind : std::uint8_t
    {
        TableIdle = 0,

        HandStarted,

        Acted,

        HandCompleted,
    };

    struct StepOutcome final
    {
        StepKind kind{};

        bool prompted = false;

        SeatId seat{};

        Action action{};

        Chips staked{};

        Chips betBefore{};

        bool allIn = false;

        Stage stage{};

        bool stageAdvanced = false;

        bool operator==(const StepOutcome &other) const = default;
    };

}
