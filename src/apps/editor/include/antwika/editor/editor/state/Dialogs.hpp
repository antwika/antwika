#pragma once

#include <optional>
#include <string>
#include <vector>

#include "antwika/editor/editor/FileDialog.hpp"
#include "antwika/editor/ui/MenuBar.hpp"

namespace antwika::editor
{

    struct Dialogs final
    {
        std::optional<Menu> openMenu;

        std::optional<FileDialog> fileDialog;

        bool quitConfirmOpen = false;

        std::vector<std::string> folderEntries;

        std::vector<std::string> mapEntries;
    };

}
