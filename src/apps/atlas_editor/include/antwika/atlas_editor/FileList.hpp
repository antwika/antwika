#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace antwika::atlas_editor
{

    struct FileEntry final
    {
        std::string name;

        bool directory = false;

        [[nodiscard]] bool operator==(const FileEntry &other) const =
            default;
    };

    inline constexpr std::string_view kParentEntry = "..";

    /**
     * @brief Lists what a directory holds, for browsing.
     *
     * @param directory The directory to read.
     * @return The parent entry first, then the directories, then the
     *         files, each group in order; the parent entry alone for a
     *         directory that cannot be read.
     */
    [[nodiscard]] std::vector<FileEntry> entriesIn(
        const std::string &directory);

    /**
     * @brief Joins a name onto the directory it was listed from.
     *
     * @param directory The directory the name was listed from.
     * @param name The entry name, which may be the parent entry.
     * @return The joined path, with any parent steps folded away.
     */
    [[nodiscard]] std::string pathIn(
        const std::string &directory, const std::string &name);

    /**
     * @brief Names an entry as the file explorer shows it.
     *
     * @param entry The entry to name.
     * @return The entry name, with a trailing slash for a directory.
     */
    [[nodiscard]] std::string entryText(const FileEntry &entry);

}
