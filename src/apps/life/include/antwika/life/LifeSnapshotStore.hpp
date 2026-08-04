#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <antwika/console/ISnapshotStore.hpp>
#include <antwika/ecs/World.hpp>

#include "antwika/life/DragState.hpp"
#include "antwika/life/Grid.hpp"
#include "antwika/life/PointerToggleSink.hpp"
#include "antwika/life/StateDump.hpp"

namespace antwika::life
{

    /**
     * @brief This application's half of dump_state and load_state.
     *
     * console::SnapshotCommands owns the policy -- the messages, the
     * refusal while recording or replaying, the history mechanics --
     * and this owns what the state *is*: a StateDump taken from and
     * applied to the live board, carried as the opaque state object of
     * the shared envelope under this application's own magic, version
     * and migrations.
     *
     * A load stages every cell's alive bit into the World, where it
     * lands at the next commit exactly as a toggle does, puts the
     * shared DragState back, and hands the pointer sink its drag
     * bookkeeping again.
     */
    class LifeSnapshotStore final
        : public antwika::console::ISnapshotStore
    {
    public:
        /**
         * @brief Construct the store over the live board.
         * @param world Staged into by a load, read by a dump. Must
         * outlive this store.
         * @param grid Maps a cell coordinate to an entity. Must
         * outlive this store.
         * @param drag Carried by a dump, restored by a load. Must
         * outlive this store.
         * @param pointer The sink whose drag bookkeeping a dump
         * carries; absent for a run that registered none, whose dumps
         * then carry no drag bookkeeping at all. Must outlive this
         * store when present.
         */
        LifeSnapshotStore(
            World &world,
            const Grid &grid,
            DragState &drag,
            std::optional<std::reference_wrapper<PointerToggleSink>>
                pointer) noexcept;

        LifeSnapshotStore(const LifeSnapshotStore &) = delete;
        LifeSnapshotStore(LifeSnapshotStore &&) = delete;

        LifeSnapshotStore &operator=(const LifeSnapshotStore &) = delete;
        LifeSnapshotStore &operator=(LifeSnapshotStore &&) = delete;

        /**
         * @brief Write the running state to a file.
         * @param path Where to write it.
         * @param console The console's history, carried in the dump.
         * @throws console::SnapshotError If the file cannot be
         * written.
         */
        void dump(
            const std::string &path,
            const std::vector<std::string> &console) override;

        /**
         * @brief Read a file and apply the state it holds.
         * @param path The file to read.
         * @return The console history the dump carried.
         * @throws console::SnapshotError If the file is not there, is
         * not this application's dump, or holds a board of another
         * size than the running one.
         */
        [[nodiscard]] std::vector<std::string> load(
            const std::string &path) override;

    private:
        [[nodiscard]] StateDump take() const;

        void apply(const StateDump &dump);

        World &world;
        const Grid &grid;
        DragState &drag;
        std::optional<std::reference_wrapper<PointerToggleSink>> pointer;
    };

} // namespace antwika::life
