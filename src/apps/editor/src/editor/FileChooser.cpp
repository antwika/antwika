#include "antwika/editor/editor/FileChooser.hpp"

#include <algorithm>
#include <filesystem>

#include <antwika/editor/ui/MapPicker.hpp>

namespace antwika::editor
{

    void FileChooser::open(
        const std::string &mapPath,
        const std::string &startPath,
        const bool forSave)
    {
        const auto path = std::filesystem::absolute(
            mapPath.empty() ? startPath : mapPath);
        const auto folder = path.parent_path().string();

        fileDialog = FileDialog{
            .isSaveMode = forSave,
            .folder = folder,
            .fileName =
                mapPath.empty() ? std::string{} : path.filename(
                    ).string()};
        listFolder(folder);
    }

    void FileChooser::listFolder(const std::string &folder)
    {
        std::vector<std::string> names;

        folderEntries.clear();
        mapEntries.clear();

        try
        {
            for (const auto &entry :
                 std::filesystem::directory_iterator(folder))
            {
                const auto tail =
                    entry.path().filename().string();

                if (entry.is_directory())
                {
                    folderEntries.push_back(tail);
                }
                else
                {
                    names.push_back(tail);
                }
            }
        }
        catch (const std::filesystem::filesystem_error &)
        {
            return;
        }

        std::sort(folderEntries.begin(), folderEntries.end());

        if (folderEntries.size()
            > antwika::editor::kMaxPicked)
        {
            folderEntries.resize(antwika::editor::kMaxPicked);
        }

        mapEntries = antwika::editor::getFilterMapNames(names);
    }

    void FileChooser::cancel()
    {
        fileDialog.reset();
    }

    std::optional<FileChoice> FileChooser::confirm(INotices &notices)
    {
        if (!fileDialog.has_value())
        {
            return std::nullopt;
        }

        if (fileDialog->fileName.empty())
        {
            notices.showStatus("the map needs a name", true, 180);

            return std::nullopt;
        }

        const auto forSave = fileDialog->isSaveMode;
        const auto path =
            (std::filesystem::path(fileDialog->folder)
             / antwika::editor::getEnsureMapExtension(
                 fileDialog->fileName))
                .string();

        fileDialog.reset();

        return FileChoice{.isSaveMode = forSave, .path = path};
    }

}
