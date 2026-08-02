#include "antwika/tower_defence/FileScoreStore.hpp"

#include <fstream>
#include <utility>

#include "antwika/tower_defence/HighScore.hpp"
#include "antwika/tower_defence/ScoreFormatError.hpp"

namespace antwika::tower_defence
{

    FileScoreStore::FileScoreStore(std::string path)
        : path(std::move(path))
    {
    }

    std::optional<HighScore> FileScoreStore::load()
    {
        std::ifstream file(path);

        // A file that is not there is not a malformed one.
        // It is a first run, and a first run has no record yet.
        // Unchecked it would reach the parser as an empty stream.
        // Which reports "nobody has played this before" as corruption.
        if (!file.is_open())
        {
            return std::nullopt;
        }

        return readHighScore(file);
    }

    void FileScoreStore::save(const HighScore &score)
    {
        std::ofstream file(path);
        if (!file.is_open())
        {
            throw ScoreFormatError(
                "antwika::tower_defence: could not open a high score to "
                "write: " + path);
        }

        writeHighScore(score, file);

        // Flushed here rather than by the destructor, which cannot say.
        // A full disk fails on the flush, not on the open.
        file.flush();
        if (!file)
        {
            throw ScoreFormatError(
                "antwika::tower_defence: could not write a high score: "
                + path);
        }
    }

} // namespace antwika::tower_defence
