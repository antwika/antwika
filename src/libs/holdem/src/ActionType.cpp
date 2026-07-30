#include "antwika/holdem/ActionType.hpp"

#include <string_view>

namespace antwika::holdem
{

    std::string_view toString(ActionType type) noexcept
    {
        switch (type)
        {
            case ActionType::Fold:
                return "fold";
            case ActionType::Check:
                return "check";
            case ActionType::Call:
                return "call";
            case ActionType::Bet:
                return "bet";
            case ActionType::Raise:
                return "raise";
        }

        return "unknown";
    }

} // namespace antwika::holdem
