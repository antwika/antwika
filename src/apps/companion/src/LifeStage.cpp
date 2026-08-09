#include "antwika/companion/LifeStage.hpp"

namespace antwika::companion
{

    namespace
    {
        constexpr std::uint32_t kCollapseWeight = 3;
    }

    PetForm formFor(const CareRecord &care)
    {
        const auto good = care.meals + care.plays;
        const auto bad = care.disturbances + care.pesters
                         + kCollapseWeight * care.collapses;

        if (bad == 0 && good > 0)
        {
            return PetForm::Bright;
        }

        if (good >= 2 * bad)
        {
            return PetForm::Plain;
        }

        return PetForm::Scruffy;
    }

}
