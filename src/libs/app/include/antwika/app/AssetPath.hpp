#pragma once

#include <string>
#include <string_view>

namespace antwika::app
{

    /**
     * @brief The directory the running executable is in.
     *
     * Asked of the operating system rather than worked out from argv[0],
     * which holds whatever the caller happened to type and holds nothing
     * useful at all when a program was found on PATH.
     *
     * @return An absolute path to the directory holding this program.
     * @throws std::filesystem::filesystem_error If the operating system
     * will not say where the program is.
     */
    [[nodiscard]] std::string executableDirectory();

    /**
     * @brief Where a file shipped beside the executable is.
     *
     * Every application gets a directory of its own under bin/, and
     * whatever it opens is copied into it, so this is how it finds one.
     * A path baked in at configure time is the building machine's, which
     * is the running machine's right up until it is not -- and a cross
     * build's never is.
     *
     * This says nothing about the working directory, deliberately: an
     * application found its assets from wherever it was started before
     * and still does.
     *
     * @param name The file's name, as it sits beside the executable.
     * @return An absolute path to that file.
     * @throws std::filesystem::filesystem_error As executableDirectory.
     */
    [[nodiscard]] std::string assetPath(std::string_view name);

} // namespace antwika::app
