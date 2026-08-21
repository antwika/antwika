#include "antwika/gfx/ShaderSource.hpp"

namespace antwika::gfx
{

    bool ShaderSource::isComplete() const
    {
        return !vertex.empty() && !fragment.empty();
    }

}
