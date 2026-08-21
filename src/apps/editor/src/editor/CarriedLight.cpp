#include "antwika/editor/editor/CarriedLight.hpp"

#include <algorithm>
#include <string>

namespace antwika::editor
{

    bool carriesLight(const map::Character &character)
    {
        return std::ranges::find(character.components, kCarriedLightName)
               != character.components.end();
    }

    void toggleCarriedLight(map::Character &character)
    {
        const auto foundName =
            std::ranges::find(character.components, kCarriedLightName);

        if (foundName == character.components.end())
        {
            character.components.emplace_back(kCarriedLightName);

            return;
        }

        character.components.erase(foundName);
    }

}
