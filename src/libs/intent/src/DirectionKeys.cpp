#include "antwika/intent/DirectionKeys.hpp"

namespace antwika::intent
{

    float DirectionKeys::getAxisX() const noexcept
    {
        return (east ? 1.0F : 0.0F) - (west ? 1.0F : 0.0F);
    }

    float DirectionKeys::getAxisZ() const noexcept
    {
        return (south ? 1.0F : 0.0F) - (north ? 1.0F : 0.0F);
    }

}
