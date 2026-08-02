#include "antwika/game/RatingsSystem.hpp"

namespace antwika::game
{

    RatingsSystem::RatingsSystem(CityRatings &ratings) noexcept
        : ratings(ratings)
    {
    }

    void RatingsSystem::update(World &world, antwika::time::Tick)
    {
        ratings = ratingsOf(world);
    }

} // namespace antwika::game
