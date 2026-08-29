#pragma once

#include <cstdint>

namespace antwika::editor
{

    enum class FocusedField : std::uint8_t
    {
        Nothing,
        ExitTarget,
        CharacterName,
        CharacterLine,
        PlanTitle,
        PlanBody,
        ComponentValue,
        MarkerAxis,
        EntityAxis,
    };

}
