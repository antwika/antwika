#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace antwika::game
{

    /**
     * @brief What every save file in a directory is called.
     *
     * The one extension listSaveGames() looks for and saveGamePath()
     * writes, so a name shown in the picker and the file it names cannot
     * drift apart.
     */
    inline constexpr std::string_view kSaveExtension = ".save.json";

    /**
     * @brief Get the file a named save lives in.
     * @param directory The directory saves are kept in.
     * @param name The save's name, as the picker shows it.
     * @return The path to read or write.
     */
    [[nodiscard]] std::string saveGamePath(
        std::string_view directory, std::string_view name);

    /**
     * @brief List the saves a directory holds, by name.
     *
     * **Read once, at startup, and never from inside the tick path.**
     * Listing a directory is I/O whose answer can differ between a live
     * run and a replay of it -- another process may have written a file
     * in between -- and a UI laid out from a list of different length
     * would resolve a recorded click to a different option. So the app
     * reads this before the loop starts and holds the answer in
     * SaveLoadState for the run, where it is fixed. What the session
     * itself writes is added to that list by SaveLoadSink, which is a
     * consequence of a recorded click and so regenerates identically.
     *
     * Sorted by name, and not by whatever order the filesystem hands
     * entries out in: which option a click lands on is a function of the
     * order, and a directory iterator promises none.
     *
     * @param directory The directory to look in.
     * @return Every save's name, ascending. A directory that is not
     * there is not an error -- a session with nothing saved yet is the
     * ordinary case, and it holds no saves.
     */
    [[nodiscard]] std::vector<std::string> listSaveGames(
        std::string_view directory);

} // namespace antwika::game
