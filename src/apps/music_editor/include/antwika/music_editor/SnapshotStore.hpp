#pragma once

#include <string>
#include <vector>

#include <antwika/console/ISnapshotStore.hpp>

#include "antwika/music_editor/EditorState.hpp"
#include "antwika/music_editor/Playback.hpp"
#include "antwika/music_editor/Score.hpp"

namespace antwika::music_editor
{

    /**
     * @brief This application's half of dump_state and load_state.
     *
     * Dumping takes the EditorState wholesale and what the playback
     * remembers; loading assigns the state back, re-reads the score
     * from the restored text, and then stands the playback at the
     * remembered instant -- in that order, because restore() sizes its
     * pool to the dump's voice count and the score is what makes that
     * count the document's again on the very next tick.
     *
     * **The simulation restores; the audible tail does not.** A load
     * silences everything sounding, and the restored instant's notes
     * re-derive as the next ticks re-trigger them.
     *
     * A line that would not parse when the dump was taken kept playing
     * its last-good chain live, and that chain is in no dump -- after
     * a load such a line is silent until its text next parses.
     */
    class MusicSnapshotStore final
        : public antwika::console::ISnapshotStore
    {
    public:
        /**
         * @brief Construct the store over the session it snapshots.
         * @param state The pane; must outlive this object.
         * @param score What the text parses into; must outlive this.
         * @param playback The clocks and voices; must outlive this.
         */
        MusicSnapshotStore(
            EditorState &state,
            Score &score,
            Playback &playback) noexcept;

        /**
         * @brief Write the running session to a file.
         * @param path Where to write it.
         * @param console The console's history, carried in the dump.
         * @throws antwika::console::SnapshotError If the file cannot
         * be written.
         */
        void dump(
            const std::string &path,
            const std::vector<std::string> &console) override;

        /**
         * @brief Read a file and stand the session at its instant.
         * @param path The file to read.
         * @return The console history the dump carried.
         * @throws antwika::console::SnapshotError If the file is not
         * there, is not this application's dump, or cannot be applied.
         */
        [[nodiscard]] std::vector<std::string> load(
            const std::string &path) override;

    private:
        EditorState &state;
        Score &score;
        Playback &playback;
    };

} // namespace antwika::music_editor
