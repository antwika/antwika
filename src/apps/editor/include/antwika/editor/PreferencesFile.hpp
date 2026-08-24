#pragma once

#include <string>

#include "antwika/editor/Preferences.hpp"

namespace antwika::editor
{

    [[nodiscard]] std::string getPreferencesPath(const std::string &mapPath);

    [[nodiscard]] Preferences getLoadPreferences(const std::string &mapPath);

    void savePreferences(
        const std::string &mapPath, const Preferences &preferences);

}
