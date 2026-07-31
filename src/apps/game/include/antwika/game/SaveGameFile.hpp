#pragma once

#include <istream>
#include <ostream>
#include <string>

#include "antwika/game/SaveGame.hpp"

namespace antwika::game
{

    /**
     * @brief Write a save to a stream as one JSON document.
     *
     * Indented, unlike a recorded replay: a save is a handful of
     * kilobytes that somebody may well want to read or hand-edit, where
     * a recording is a long machine-written log nobody opens.
     *
     * @param save The state to write.
     * @param out The stream to write it to.
     */
    void writeSaveGame(const SaveGame &save, std::ostream &out);

    /**
     * @brief Read a save from a stream.
     * @param in The stream to read from.
     * @return The decoded state.
     * @throws SaveFormatError If the stream is not valid JSON, or is not
     * a save document this build can read.
     */
    [[nodiscard]] SaveGame readSaveGame(std::istream &in);

    /**
     * @brief Write a save to a file.
     *
     * The path comes from the caller and is never defaulted here: which
     * file a session is saved to is an application's decision, and a
     * library that picked one would pick the same one for every session.
     *
     * @param save The state to write.
     * @param path Where to write it.
     * @throws SaveFormatError If the file cannot be opened, or if the
     * bytes cannot be written once it is. A save is written in one go, so
     * failing quietly here loses the whole session.
     */
    void saveGameFile(const SaveGame &save, const std::string &path);

    /**
     * @brief Read a save from a file.
     * @param path The file to read.
     * @return The decoded state.
     * @throws SaveFormatError If the file cannot be opened at all, or
     * does not hold a save document this build can read. The two say so
     * differently: a file that is not there is not a malformed one.
     */
    [[nodiscard]] SaveGame loadGameFile(const std::string &path);

} // namespace antwika::game
