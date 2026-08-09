#pragma once

#include <string>
#include <vector>

#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/CityRatings.hpp"
#include "antwika/game/GameState.hpp"
#include "antwika/game/KeyBindings.hpp"
#include "antwika/game/KeyboardLayout.hpp"
#include "antwika/game/SceneSnapshot.hpp"
#include "antwika/game/Toolbar.hpp"

namespace antwika::game
{

    struct GameSummary final
    {
        GameState state;
        std::vector<Cell> paths;
        std::vector<WalkerView> walkers;

        std::vector<BuildingView> buildings;

        std::vector<RuinView> ruins;

        Camera camera;

        CityRatings ratings;

        std::vector<std::string> console;

        KeyboardLayout keyboard{kDefaultKeyboardLayout};

        KeyBindings bindings;

        [[nodiscard]] bool operator==(
            const GameSummary &other) const = default;
    };

}
