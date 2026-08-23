#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace antwika::io
{

    struct FileEntry final
    {
        std::string name;

        bool directory = false;

        [[nodiscard]] bool operator==(const FileEntry &other) const =
            default;
    };

    inline constexpr std::string_view kParentEntry = "..";

    [[nodiscard]] std::vector<FileEntry> entriesIn(
        const std::string &directory);

    [[nodiscard]] std::string pathIn(
        const std::string &directory, const std::string &name);

    [[nodiscard]] std::string getEntryText(const FileEntry &entry);

}
