#include "antwika/music_editor/ScoreFiles.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>

namespace antwika::music_editor
{

    namespace
    {
        [[nodiscard]] bool nameCharacter(const char letter) noexcept
        {
            return (letter >= 'a' && letter <= 'z')
                || (letter >= 'A' && letter <= 'Z')
                || (letter >= '0' && letter <= '9') || letter == '-'
                || letter == '_';
        }
    }

    std::string scorePath(
        const std::string_view directory, const std::string_view name)
    {
        return std::string(directory) + "/" + std::string(name)
               + std::string(kScoreExtension);
    }

    std::string safeScoreName(const std::string_view name)
    {
        std::string safe;

        for (const char letter : name)
        {
            if (nameCharacter(letter))
            {
                safe.push_back(letter);
            }
        }

        return safe;
    } // GCOVR_EXCL_LINE

    std::vector<std::string> listScores(const std::string_view directory)
    {
        std::vector<std::string> names;

        std::error_code failed;
        const std::filesystem::directory_iterator entries(
            std::filesystem::path(directory), failed);

        if (failed)
        {
            return names;
        }

        for (const auto &entry : entries)
        {
            const std::string file = entry.path().filename().string();

            if (file.size() <= kScoreExtension.size()
                || !file.ends_with(kScoreExtension))
            {
                continue;
            }

            names.push_back(
                file.substr(0, file.size() - kScoreExtension.size()));
        }

        std::sort(names.begin(), names.end());
        return names;
    }

    void saveScore(const std::string &path, const std::string_view text)
    {
        const auto parent = std::filesystem::path(path).parent_path();

        std::error_code failed;
        std::filesystem::create_directories(parent, failed);

        if (failed)
        {
            throw ScoreFileError(
                "no directory would take the score at " + path);
        }

        std::ofstream out(path, std::ios::binary | std::ios::trunc);

        out.write(
            text.data(), static_cast<std::streamsize>(text.size()));

        if (!out)
        {
            throw ScoreFileError("the score would not write to " + path);
        }
    }

    std::string loadScore(const std::string &path)
    {
        std::ifstream in(path, std::ios::binary);

        std::ostringstream text;
        text << in.rdbuf();

        if (!in)
        {
            throw ScoreFileError("no score reads back from " + path);
        }

        return text.str();
    } // GCOVR_EXCL_LINE

}
