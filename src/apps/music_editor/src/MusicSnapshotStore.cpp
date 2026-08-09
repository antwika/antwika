#include "antwika/music_editor/MusicSnapshotStore.hpp"

#include "antwika/music_editor/StateDump.hpp"

namespace antwika::music_editor
{

    MusicSnapshotStore::MusicSnapshotStore(
        EditorState &state, Score &score, Playback &playback) noexcept
        : antwika::console::IJsonSnapshotStore<StateDumpError>(
              {.magic = kStateDumpMagic,
               .version = kStateDumpVersion},
              "antwika music editor state dump document",
              standardStateDumpMigrations),
          state(state),
          score(score),
          playback(playback)
    {
    }

    nlohmann::json MusicSnapshotStore::takeState(const std::string &)
    {
        EditorDump dump;

        dump.editor = state;
        dump.playback = playback.remember();

        return editorDumpToJson(dump);
    }

    void MusicSnapshotStore::applyState(
        const std::string &, const nlohmann::json &dumped)
    {
        const auto dump = editorDumpFromJson(dumped);

        state = dump.editor;
        score.read(state.source);

        playback.restore(dump.playback);
    }

}
