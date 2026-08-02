#include "antwika/pattern/Hap.hpp"

namespace antwika::pattern
{

    bool Hap::hasOnset() const noexcept
    {
        return whole.has_value() && whole->begin() == part.begin();
    }

} // namespace antwika::pattern
