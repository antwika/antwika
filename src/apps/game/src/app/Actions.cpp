#include "antwika/game/app/Actions.hpp"

#include <string>

#include <antwika/input/Key.hpp>

namespace antwika::game
{

    input::ActionMap getDefaultActions()
    {
        input::ActionMap actions;

        actions.bind(std::string(kWalkNorth), input::Key::W);
        actions.bind(std::string(kWalkNorth), input::Key::ArrowUp);
        actions.bind(std::string(kWalkSouth), input::Key::S);
        actions.bind(std::string(kWalkSouth), input::Key::ArrowDown);
        actions.bind(std::string(kWalkWest), input::Key::A);
        actions.bind(std::string(kWalkWest), input::Key::ArrowLeft);
        actions.bind(std::string(kWalkEast), input::Key::D);
        actions.bind(std::string(kWalkEast), input::Key::ArrowRight);
        actions.bind(std::string(kRun), input::Key::LeftShift);
        actions.bind(std::string(kRun), input::Key::RightShift);
        actions.bind(std::string(kLeave), input::Key::Escape);

        return actions;
    }

}
