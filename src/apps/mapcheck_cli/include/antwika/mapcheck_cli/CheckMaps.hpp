#pragma once

#include <filesystem>
#include <ostream>
#include <vector>

namespace antwika::mapcheck_cli
{

    /**
     * @brief Loads each map file, validates it, and prints the findings.
     *
     * @param paths The map JSON files to load and validate.
     * @param out The stream every finding and load error is printed to.
     * @return True when every file loaded and no finding was printed.
     */
    [[nodiscard]] bool checkMaps(
        const std::vector<std::filesystem::path> &paths,
        std::ostream &out);

}
