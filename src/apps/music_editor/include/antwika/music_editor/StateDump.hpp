#pragma once

#include <cstdint>
#include <string_view>

#include <nlohmann/json.hpp>

#include <antwika/replay/MigrationChain.hpp>

#include "antwika/music_editor/EditorState.hpp"
#include "antwika/music_editor/Playback.hpp"

namespace antwika::music_editor
{

    /**
     * @brief What every dump of this application says it is.
     */
    inline constexpr std::string_view kStateDumpMagic =
        "antwika-music-editor-state-dump";

    /**
     * @brief The dump revision this build writes.
     */
    inline constexpr std::uint32_t kStateDumpVersion = 1;

    /**
     * @brief The running session, as the console's dump_state takes it.
     *
     * **The document plus the clocks under it.** The EditorState is
     * already everything a keystroke's meaning depends on -- the text,
     * the caret, the scroll, the modal, the speed choice -- and the
     * PlaybackMemory is where the musical clock stood against the
     * device.  Coming back to a dump therefore means the same pane
     * over the same bar of the same score.
     *
     * What it deliberately does not carry, each regenerated instead:
     * the synth's voice pool and the notes already sounding, which are
     * the audible tail restore() cuts; every pattern::Pattern, which
     * Score::read() re-derives from the text; the waveform image
     * cache, which is the render side's; and the sink's mirrored
     * clipboard, which is an outward write no tick reads back.
     */
    struct EditorDump
    {
        /** @brief The pane, exactly as the editor held it. */
        EditorState editor;

        /** @brief The clocks, exactly as the playback held them. */
        PlaybackMemory playback;

        /**
         * @brief Compare two dumps.
         * @param other The dump to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(
            const EditorDump &other) const = default;
    };

    /**
     * @brief Build the chain that brings an old dump document up.
     *
     * Empty: version 1 is the first shape this dump has ever had.
     *
     * @return The chain.
     */
    [[nodiscard]] antwika::replay::MigrationChain
    standardStateDumpMigrations();

    /**
     * @brief Encode a dump as the envelope's opaque state object.
     * @param dump The state to encode.
     * @return The state object, magic-free: the envelope stamps the
     * document -- see console::SnapshotFormat.
     */
    [[nodiscard]] nlohmann::json editorDumpToJson(
        const EditorDump &dump);

    /**
     * @brief Decode the envelope's state object, validating it first.
     * @param state The state object a snapshot carried.
     * @return The decoded state.
     * @throws StateDumpError If the object is not a state this build
     * can read.
     */
    [[nodiscard]] EditorDump editorDumpFromJson(
        const nlohmann::json &state);

} // namespace antwika::music_editor
