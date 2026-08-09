#include "antwika/poker/AgentStyle.hpp"

#include <string_view>

namespace antwika::poker
{

    std::string_view toString(AgentStyle style) noexcept
    {
        switch (style)
        {
            case AgentStyle::Tight:
                return "tight";
            case AgentStyle::Balanced:
                return "balanced";
            case AgentStyle::Aggressive:
                return "aggressive";
        }

        return "unknown";
    }

}
