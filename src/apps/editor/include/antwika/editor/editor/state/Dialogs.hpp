#pragma once

#include <optional>

#include "antwika/editor/ui/MenuBar.hpp"

namespace antwika::editor
{

    struct Dialogs final
    {
        std::optional<Menu> openMenu;

        bool quitConfirmOpen = false;
    };

}
