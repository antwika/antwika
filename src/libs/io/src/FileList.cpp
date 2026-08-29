#include "antwika/io/FileList.hpp"

#include <algorithm>
#include <filesystem>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace antwika::io
{

    namespace
    {
        void appendSorted(
            std::vector<FileEntry> &listedEntry,
            std::vector<std::string> names,
            const bool directory)
        {
            std::ranges::sort(names);

            for (auto &name : names)
            {
                FileEntry entry;
                entry.name = std::move(name);
                entry.directory = directory;

                listedEntry.push_back(std::move(entry));
            }
        }
    }

    std::vector<FileEntry> entriesIn(const std::string &directory)
    {
        std::vector<FileEntry> listedEntry;

        FileEntry upEntry;
        upEntry.name = kParentEntry;
        upEntry.directory = true;

        listedEntry.push_back(std::move(upEntry));

        std::error_code openCode;
        std::filesystem::directory_iterator entries(directory, openCode);

        if (openCode)
        {
            return listedEntry;
        }

        std::vector<std::string> folders;
        std::vector<std::string> files;

        const std::filesystem::directory_iterator pastEnd;

        std::error_code stepCode;

        while (entries != pastEnd && !stepCode)
        {
            const auto &entry = *entries;

            std::error_code statusCode;

            if (entry.is_directory(statusCode))
            {
                folders.push_back(entry.path().filename().string());
            }
            else if (entry.is_regular_file(statusCode))
            {
                files.push_back(entry.path().filename().string());
            }

            entries.increment(stepCode);
        }

        appendSorted(listedEntry, std::move(folders), true);
        appendSorted(listedEntry, std::move(files), false);

        return listedEntry;
    } // GCOVR_EXCL_LINE

    std::string pathIn(
        const std::string &directory, const std::string &name)
    {
        const std::filesystem::path joinedPath =
            (std::filesystem::path{directory} / name)
                .lexically_normal();

        const std::filesystem::path fileName =
            joinedPath.filename().empty() ? joinedPath.parent_path(
                ) : joinedPath;

        return fileName.string();
    } // GCOVR_EXCL_LINE

    std::string getEntryText(const FileEntry &entry)
    {
        std::string shownName = entry.name;

        if (entry.directory)
        {
            shownName += '/';
        }

        return shownName;
    } // GCOVR_EXCL_LINE

}
