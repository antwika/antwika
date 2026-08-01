#pragma once

#include "antwika/companion/Lineage.hpp"
#include "antwika/companion/Pet.hpp"

namespace antwika::companion
{

    /**
     * @brief Everything one file holds: a companion, and the record of
     * the ones before it.
     *
     * Two values rather than one flattened struct, because they have
     * different lifetimes -- `Pet::revive()` replaces the first whole
     * and leaves the second standing, which is the entire point of the
     * second existing.
     */
    struct CompanionMemory
    {
        PetMemory pet;
        LineageMemory lineage;

        /**
         * @brief Compare two documents field by field.
         * @param other The document to compare against.
         * @return Whether both halves match.
         */
        [[nodiscard]] bool operator==(const CompanionMemory &other) const
            = default;
    };

} // namespace antwika::companion
