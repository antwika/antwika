#pragma once

#include <optional>
#include <string>

#include <antwika/app/FileSnapshotStore.hpp>

#include "antwika/tower_defence/IScoreStore.hpp"
#include "antwika/tower_defence/ScoreFormatError.hpp"

namespace antwika::tower_defence
{

    /**
     * @brief Keeps the record in one file.
     *
     * The only class in this application that opens anything, which is
     * what lets every other one be exercised with no file on disk.
     * It holds the path and nothing else: what a document looks like is
     * HighScore's, and both halves are split apart so a round trip
     * through the format is assertable with no filesystem at all.
     *
     * The path comes from the caller and is never defaulted here: which
     * file a record lives in is the application's decision, and a class
     * that picked one would pick the same one for every run.
     */
    class FileScoreStore final : public IScoreStore
    {
    public:
        /**
         * @brief Construct the store over the file it uses.
         * @param path Where the record is read from and written to.
         */
        explicit FileScoreStore(std::string path);

        /**
         * @brief Read the best any earlier run reached.
         * @return What it was, or nothing when the file is not there --
         * a first run, which is an ordinary answer rather than a
         * failure and starts from a best of zero.
         * @throws ScoreFormatError If the file is there and is not a
         * record this build can read.
         */
        [[nodiscard]] std::optional<HighScore> load() override;

        /**
         * @brief Write the record out.
         * @param score The record to keep.
         * @throws ScoreFormatError If the file cannot be opened, or if
         * the bytes cannot be written once it is.
         */
        void save(const HighScore &score) override;

    private:
        antwika::app::FileSnapshotStore<HighScore, ScoreFormatError> file;
    };

} // namespace antwika::tower_defence
