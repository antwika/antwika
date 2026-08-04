#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include <antwika/console/JsonSnapshotStore.hpp>

#include "antwika/music_editor/EditorState.hpp"
#include "antwika/music_editor/Playback.hpp"
#include "antwika/music_editor/Score.hpp"
#include "antwika/music_editor/StateDumpError.hpp"

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
        : public antwika::console::JsonSnapshotStore<StateDumpError>
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

} // namespace antwika::music_editor
