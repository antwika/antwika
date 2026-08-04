#include "antwika/music_editor/SnapshotStore.hpp"

#include <antwika/console/SnapshotError.hpp>
#include <antwika/console/SnapshotFormat.hpp>

#include "antwika/music_editor/StateDump.hpp"
#include "antwika/music_editor/StateDumpError.hpp"

namespace antwika::music_editor
{

    namespace
    {
        const antwika::console::SnapshotFormat &dumpFormat()
        {
            // The excluded closing line carries the static guard.
            // Its concurrency arms are unreachable one-threaded.
            static const antwika::console::SnapshotFormat format(
                {.magic = kStateDumpMagic,
                 .version = kStateDumpVersion},
                "antwika music editor state dump document",
                standardStateDumpMigrations); // GCOVR_EXCL_LINE
            return format;
        }
    } // namespace

    MusicSnapshotStore::MusicSnapshotStore(
        EditorState &state, Score &score, Playback &playback) noexcept
        : state(state), score(score), playback(playback)
    {
    }

    void MusicSnapshotStore::dump(
        const std::string &path,
        const std::vector<std::string> &console)
    {
        dumpFormat().write(
            antwika::console::Snapshot{
                .console = console,
                .state = editorDumpToJson(
                    EditorDump{
                        .editor = state,
                        .playback = playback.remember()})},
            path);

        // The excluded line is the local snapshot's unwind destructor.
        // Nothing after its construction throws but the write itself.
    } // GCOVR_EXCL_LINE

    std::vector<std::string> MusicSnapshotStore::load(
        const std::string &path)
    {
        auto snapshot = dumpFormat().read(path);

        try
        {
            const auto dump = editorDumpFromJson(snapshot.state);

            // The state wholesale, then the score it parses into.
            // The sink would re-read it on the very next tick anyway.
            // Reading it here makes the voice count right at once.
            state = dump.editor;
            score.read(state.source);

            // Last, once the text the voices come from is in place.
            playback.restore(dump.playback);
        }
        // The codec and the restore promise this app's own error.
        // What this seam promises is console::SnapshotError.
        catch (const StateDumpError &failed)
        {
            throw antwika::console::SnapshotError(failed.what());
        }

        return snapshot.console;
    }

} // namespace antwika::music_editor
