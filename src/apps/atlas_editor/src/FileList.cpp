#include "antwika/atlas_editor/FileList.hpp"

#include <algorithm>
#include <filesystem>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace antwika::atlas_editor
{

    namespace
    {
        void appendSorted(
            std::vector<FileEntry> &listed,
            std::vector<std::string> named,
            const bool directory)
        {
            std::ranges::sort(named);

            for (auto &name : named)
            {
                FileEntry entry;
                entry.name = std::move(name);
                entry.directory = directory;

                listed.push_back(std::move(entry));
            }
        }
    }

    std::vector<FileEntry> entriesIn(const std::string &directory)
    {
        std::vector<FileEntry> listed;

        FileEntry up;
        up.name = kParentEntry;
        up.directory = true;

        listed.push_back(std::move(up));

        std::error_code failed;

        const std::filesystem::directory_iterator found(
            directory, failed);

        if (failed)
        {
            return listed;
        }

        std::vector<std::string> folders;
        std::vector<std::string> files;

        for (const auto &entry : found)
        {
            if (entry.is_directory(failed))
            {
                folders.push_back(entry.path().filename().string());
            }
            else if (entry.is_regular_file(failed))
            {
                files.push_back(entry.path().filename().string());
            }
        }

        appendSorted(listed, std::move(folders), true);
        appendSorted(listed, std::move(files), false);

        return listed;
    } // GCOVR_EXCL_LINE

    std::string pathIn(
        const std::string &directory, const std::string &name)
    {
        const std::filesystem::path joined =
            (std::filesystem::path{directory} / name)
                .lexically_normal();

        const std::filesystem::path named =
            joined.filename().empty() ? joined.parent_path() : joined;

        return named.string();
    } // GCOVR_EXCL_LINE

    std::string entryText(const FileEntry &entry)
    {
        std::string shown = entry.name;

        if (entry.directory)
        {
            shown += '/';
        }

        return shown;
    } // GCOVR_EXCL_LINE

}
