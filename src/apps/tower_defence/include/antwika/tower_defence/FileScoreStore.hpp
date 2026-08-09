#pragma once

#include <optional>
#include <string>

#include <antwika/app/FileSnapshotStore.hpp>

#include "antwika/tower_defence/IScoreStore.hpp"
#include "antwika/tower_defence/ScoreFormatError.hpp"

namespace antwika::tower_defence
{

    class FileScoreStore final : public IScoreStore
    {
    public:
        explicit FileScoreStore(std::string path);

        [[nodiscard]] std::optional<HighScore> load() override;

        void save(const HighScore &score) override;

    private:
        antwika::app::FileSnapshotStore<HighScore, ScoreFormatError> file;
    };

}
