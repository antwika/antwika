#pragma once

#include <cstdint>
#include <map>
#include <string>

namespace antwika::map_editor
{

    struct MapEditorConfig final
    {
        std::uint32_t uiScale = 3;
        bool fullscreen = false;

        /**
         * @brief The hotkey bindings, action name to key name.
         *
         * Ensures: unknown or missing entries fall back to the
         *          compiled-in defaults.
         */
        std::map<std::string, std::string> keys{};
    };

}
