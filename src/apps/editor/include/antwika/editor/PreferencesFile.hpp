#pragma once

#include <string>

#include "antwika/editor/Preferences.hpp"

namespace antwika::editor
{

    [[nodiscard]] std::string getPreferencesPath(const std::string &mapPath);

    /**
     * @brief The preferences beside a map, where restingPreferences
     * stands for whatever the file leaves unsaid.
     */
    [[nodiscard]] Preferences getLoadPreferences(
        const std::string &mapPath,
        const Preferences &restingPreferences = Preferences{});

    void savePreferences(
        const std::string &mapPath, const Preferences &preferences);

}
