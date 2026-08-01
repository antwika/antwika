#include "antwika/companion/LifeStage.hpp"

namespace antwika::companion
{

    namespace
    {
        // What one collapse is worth against a meal or a game.
        // Three, since it is the one that takes something back.
        constexpr std::uint32_t kCollapseWeight = 3;
    } // namespace

    PetForm formFor(const CareRecord &care)
    {
        const auto good = care.meals + care.plays;
        const auto bad = care.disturbances + care.pesters
                         + kCollapseWeight * care.collapses;

        // A companion nobody ever did anything to is not Bright.
        // It is merely untested, which is what Plain means below.
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

} // namespace antwika::companion
