#include "antwika/intent/DirectionKeys.hpp"

namespace antwika::intent
{

    float DirectionKeys::axisX() const noexcept
    {
        return (east ? 1.0F : 0.0F) - (west ? 1.0F : 0.0F);
    }

    float DirectionKeys::axisZ() const noexcept
    {
        return (south ? 1.0F : 0.0F) - (north ? 1.0F : 0.0F);
    }

}
