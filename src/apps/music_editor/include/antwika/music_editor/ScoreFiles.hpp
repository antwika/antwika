#pragma once

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace antwika::music_editor
{

    class ScoreFileError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    inline constexpr std::string_view kScoreExtension{".score"};

    [[nodiscard]] std::string scorePath(
        std::string_view directory, std::string_view name);

    [[nodiscard]] std::string safeScoreName(std::string_view name);

    [[nodiscard]] std::vector<std::string> listScores(
        std::string_view directory);

    void saveScore(const std::string &path, std::string_view text);

    [[nodiscard]] std::string loadScore(const std::string &path);

}
