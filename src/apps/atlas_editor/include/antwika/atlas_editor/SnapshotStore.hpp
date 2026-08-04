#pragma once

#include <stdexcept>
#include <string>

#include <nlohmann/json.hpp>

#include <antwika/console/JsonSnapshotStore.hpp>

#include "antwika/atlas_editor/EditorState.hpp"

namespace antwika::atlas_editor
{

    /**
     * @brief This application's half of dump_state and load_state.
     *
     * The sheet and the clipboard are bitmaps, and a bitmap inline in
     * the JSON would be megabytes of numbers -- so each is written as
     * a PNG beside the document (`<dump>.sheet.png`, and
     * `<dump>.clipboard.png` when anything is in hand), and the
     * document binds them by fingerprintOf().
     * A load whose PNG does not answer the fingerprint the JSON names
     * is refused whole: a dump edited by hand is not repaired.
     *
     * Everything that can go wrong -- a PNG that will not read, a
     * document naming a tool this build lacks -- leaves this seam as
     * console::SnapshotError, which is the type the seam promises.
     * Which is why std::runtime_error is what it names as its own
     * category: every failure either half can meet is one of its own.
     */
    class EditorSnapshotStore final
        : public console::JsonSnapshotStore<std::runtime_error>
    {
    public:
        /**
         * @brief Construct the store over the session it snapshots.
         * @param state The state dumped and restored. Must outlive
         * this store.
         */
        explicit EditorSnapshotStore(EditorState &state) noexcept;

    private:
        // The PNGs go beside the document.
        // So both halves read the path they are handed.
        [[nodiscard]] nlohmann::json takeState(
            const std::string &path) override;

        void applyState(
            const std::string &path,
            const nlohmann::json &dumped) override;

        EditorState &state;
    };

} // namespace antwika::atlas_editor
