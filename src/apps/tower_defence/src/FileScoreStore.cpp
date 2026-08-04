#include "antwika/tower_defence/FileScoreStore.hpp"

#include <utility>

#include "antwika/tower_defence/HighScore.hpp"

namespace antwika::tower_defence
{

    FileScoreStore::FileScoreStore(std::string path)
        : file(
              std::move(path),
              readHighScore,
              writeHighScore,
              "a high score")
    {
    }

    std::optional<HighScore> FileScoreStore::load()
    {
        return file.loadIfPresent();
    }

    void FileScoreStore::save(const HighScore &score)
    {
        file.store(score);
    }

} // namespace antwika::tower_defence
