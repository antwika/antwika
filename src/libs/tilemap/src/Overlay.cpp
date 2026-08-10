#include "antwika/tilemap/Overlay.hpp"

#include <string_view>

namespace antwika::tilemap
{

    std::string_view toString(Overlay overlay) noexcept
    {
        switch (overlay)
        {
            case Overlay::None:
                return "none";
            case Overlay::Bridge:
                return "bridge";
        }

        return "unknown";
    }

}
