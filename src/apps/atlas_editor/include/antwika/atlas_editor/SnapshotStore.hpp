#pragma once

#include <string>
#include <vector>

#include <antwika/console/ISnapshotStore.hpp>

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
     */
    class EditorSnapshotStore final : public console::ISnapshotStore
    {
    public:
        /**
         * @brief Construct the store over the session it snapshots.
         * @param state The state dumped and restored. Must outlive
         * this store.
         */
        explicit EditorSnapshotStore(EditorState &state) noexcept;

        /**
         * @brief Write the running state beside its PNGs.
         * @param path Where the JSON document goes; the PNG paths are
         * derived from it.
         * @param console The console's history, carried in the dump.
         * @throws console::SnapshotError If any of the files cannot
         * be written.
         */
        void dump(
            const std::string &path,
            const std::vector<std::string> &console) override;

        /**
         * @brief Read a dump and put its state back into the session.
         * @param path The JSON document to read.
         * @return The console history the dump carried.
         * @throws console::SnapshotError If any file is missing, the
         * document is not this application's dump, or a PNG does not
         * answer the fingerprint the document names.
         */
        [[nodiscard]] std::vector<std::string> load(
            const std::string &path) override;

    private:
        EditorState &state;
    };

} // namespace antwika::atlas_editor
