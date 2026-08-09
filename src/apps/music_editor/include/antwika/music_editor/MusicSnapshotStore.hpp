#pragma once

#include <nlohmann/json.hpp>

#include <string>

#include <antwika/console/IJsonSnapshotStore.hpp>

#include "antwika/music_editor/EditorState.hpp"
#include "antwika/music_editor/Playback.hpp"
#include "antwika/music_editor/Score.hpp"
#include "antwika/music_editor/StateDumpError.hpp"

namespace antwika::music_editor
{

    class MusicSnapshotStore final
        : public antwika::console::IJsonSnapshotStore<StateDumpError>
    {
    public:
        MusicSnapshotStore(
            EditorState &state,
            Score &score,
            Playback &playback) noexcept;

    private:
        [[nodiscard]] nlohmann::json takeState(
            const std::string &path) override;

        void applyState(
            const std::string &path,
            const nlohmann::json &dumped) override;

        EditorState &state;
        Score &score;
        Playback &playback;
    };

}
