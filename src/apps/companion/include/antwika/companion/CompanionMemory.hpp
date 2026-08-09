#pragma once

#include "antwika/companion/Lineage.hpp"
#include "antwika/companion/Pet.hpp"

namespace antwika::companion
{

    struct CompanionMemory final
    {
        PetMemory pet;
        LineageMemory lineage;

        [[nodiscard]] bool operator==(const CompanionMemory &other) const
            = default;
    };

}
