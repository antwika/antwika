#pragma once

#include <optional>
#include <string>
#include <vector>

#include "antwika/editor/editor/FileChoice.hpp"
#include "antwika/editor/editor/FileDialog.hpp"
#include "antwika/editor/view/INotices.hpp"

namespace antwika::editor
{

    class FileChooser final
    {
    public:
        std::optional<FileDialog> fileDialog;

        std::vector<std::string> folderEntries;

        std::vector<std::string> mapEntries;

        void open(
            const std::string &mapPath,
            const std::string &startPath,
            bool forSave);

        void listFolder(const std::string &folder);

        void cancel();

        [[nodiscard]] std::optional<FileChoice> confirm(INotices &notices);
    };

}
