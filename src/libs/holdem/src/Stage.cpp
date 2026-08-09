#include "antwika/holdem/Stage.hpp"

#include <string_view>

namespace antwika::holdem
{

    std::string_view toString(Stage stage) noexcept
    {
        switch (stage)
        {
            case Stage::PreFlop:
                return "pre-flop";
            case Stage::Flop:
                return "flop";
            case Stage::Turn:
                return "turn";
            case Stage::River:
                return "river";
            case Stage::Showdown:
                return "showdown";
        }

        return "unknown";
    }

}
