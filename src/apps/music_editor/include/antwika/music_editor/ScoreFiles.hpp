#pragma once

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace antwika::music_editor
{

    /**
     * @brief Thrown when a score file cannot be written or read.
     *
     * Its own type rather than ScoreError, which is a refused *line*:
     * a disk that would not take the document is a different failure
     * category, and the sink shows one in the modal and the other
     * under the pane.
     */
    class ScoreFileError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    /**
     * @brief The one extension listScores() looks for and scorePath()
     * appends.
     */
    inline constexpr std::string_view kScoreExtension{".score"};

    /**
     * @brief Where a named score lives under a directory.
     * @param directory Where scores are kept.
     * @param name The score's name, with no extension.
     * @return The path, as directory/name plus kScoreExtension.
     */
    [[nodiscard]] std::string scorePath(
        std::string_view directory, std::string_view name);

    /**
     * @brief Keep only what a score's name may be made of.
     *
     * Letters, digits, dashes and underscores; everything else is
     * dropped rather than escaped, so what the load list later shows
     * is exactly what the file is called -- and a separator can never
     * write outside the scores directory.
     *
     * @param name What was typed.
     * @return The name a file may carry, possibly empty.
     */
    [[nodiscard]] std::string safeScoreName(std::string_view name);

    /**
     * @brief List the scores a directory holds, sorted by name.
     *
     * **Read once at startup and never inside the tick path**, exactly
     * as apps/game reads its saves: which button a click lands on is a
     * function of this list, so a replay must reach the same one, and
     * a directory read mid-run would answer with the replaying
     * machine's files.  What a session itself saves is added to the
     * state's copy of the list instead, which regenerates on replay.
     *
     * A directory that is not there is a session with no scores.
     *
     * @param directory Where scores are kept.
     * @return The names, without extensions, in ascending order.
     */
    [[nodiscard]] std::vector<std::string> listScores(
        std::string_view directory);

    /**
     * @brief Write a score where scorePath() says.
     *
     * The directory is made if it is missing, so the first save of a
     * fresh checkout works.
     *
     * @param path Where to write it.
     * @param text The whole document.
     * @throws ScoreFileError If the directory or the file would not
     * take it.
     */
    void saveScore(const std::string &path, std::string_view text);

    /**
     * @brief Read a score back.
     * @param path Where scorePath() put it.
     * @return The whole document.
     * @throws ScoreFileError If there is no readable file there.
     */
    [[nodiscard]] std::string loadScore(const std::string &path);

} // namespace antwika::music_editor
