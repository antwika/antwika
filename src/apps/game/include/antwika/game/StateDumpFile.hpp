#pragma once

#include <istream>
#include <ostream>
#include <string>

#include "antwika/game/StateDump.hpp"

namespace antwika::game
{

    /**
     * @brief Write a dump to a stream as one JSON document.
     *
     * Indented, exactly as a save is: a dump is a document somebody
     * debugging a run may well want to read or hand-edit.
     *
     * @param dump The state to write.
     * @param out The stream to write it to.
     */
    void writeStateDump(const StateDump &dump, std::ostream &out);

    /**
     * @brief Read a dump from a stream.
     * @param in The stream to read from.
     * @return The decoded state.
     * @throws SaveFormatError If the stream is not valid JSON, or is
     * not a dump document this build can read.
     */
    [[nodiscard]] StateDump readStateDump(std::istream &in);

    /**
     * @brief Write a dump to a file.
     * @param dump The state to write.
     * @param path Where to write it.
     * @throws SaveFormatError If the file cannot be opened, or if the
     * bytes cannot be written once it is.
     */
    void stateDumpFile(const StateDump &dump, const std::string &path);

    /**
     * @brief Read a dump from a file.
     * @param path The file to read.
     * @return The decoded state.
     * @throws SaveFormatError If the file cannot be opened at all, or
     * does not hold a dump document this build can read.
     */
    [[nodiscard]] StateDump loadStateDump(const std::string &path);

} // namespace antwika::game
