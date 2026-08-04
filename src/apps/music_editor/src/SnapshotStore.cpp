#include "antwika/music_editor/SnapshotStore.hpp"

#include "antwika/music_editor/StateDump.hpp"

namespace antwika::music_editor
{

    MusicSnapshotStore::MusicSnapshotStore(
        EditorState &state, Score &score, Playback &playback) noexcept
        : antwika::console::JsonSnapshotStore<StateDumpError>(
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

        // The state wholesale, then the score it parses into.
        // The sink would re-read it on the very next tick anyway.
        // Reading it here makes the voice count right at once.
        state = dump.editor;
        score.read(state.source);

        // Last, once the text the voices come from is in place.
        playback.restore(dump.playback);
    }

} // namespace antwika::music_editor
