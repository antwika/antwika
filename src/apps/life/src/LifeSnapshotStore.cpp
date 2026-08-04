#include "antwika/life/LifeSnapshotStore.hpp"

#include <cstdint>
#include <set>
#include <string>
#include <utility>

#include <antwika/console/SnapshotFormat.hpp>
#include <antwika/ecs/Entity.hpp>

#include "antwika/life/Board.hpp"
#include "antwika/life/Cell.hpp"

namespace antwika::life
{

    namespace
    {
        const antwika::console::SnapshotFormat &dumpFormat()
        {
            // The excluded closing line carries the static guard.
            // Its concurrency arms are unreachable one-threaded.
            // See docs/confirming-unreachable-branches.md.
            static const antwika::console::SnapshotFormat format(
                {.magic = kStateDumpMagic,
                 .version = kStateDumpVersion},
                "antwika life state dump document",
                standardStateDumpMigrations); // GCOVR_EXCL_LINE
            return format;
        }
    } // namespace

    LifeSnapshotStore::LifeSnapshotStore(
        World &world,
        const Grid &grid,
        DragState &drag,
        std::optional<std::reference_wrapper<PointerToggleSink>>
            pointer) noexcept
        : world(world), grid(grid), drag(drag), pointer(pointer)
    {
    }

    void LifeSnapshotStore::dump(
        const std::string &path,
        const std::vector<std::string> &console)
    {
        // The excluded lines are the envelope temporary's unwind arms.
        // Only a failed allocation inside them could take one.
        // See docs/confirming-unreachable-branches.md.
        dumpFormat().write(
            // GCOVR_EXCL_START
            antwika::console::Snapshot{
                .console = console, .state = stateDumpToJson(take())},
            // GCOVR_EXCL_STOP
            path);

        // The excluded line is the local snapshot's unwind destructor.
        // Nothing after its construction throws but the write itself.
    } // GCOVR_EXCL_LINE

    std::vector<std::string> LifeSnapshotStore::load(
        const std::string &path)
    {
        auto snapshot = dumpFormat().read(path);

        try
        {
            apply(stateDumpFromJson(snapshot.state));
        }
        // The state's own reader promises StateDumpError.
        // What this seam promises is console::SnapshotError.
        // So it is rewrapped here, exactly as apps/game's store does.
        catch (const StateDumpError &failed) // GCOVR_EXCL_LINE
        {
            throw antwika::console::SnapshotError(failed.what());
        }

        return snapshot.console;
    }

    StateDump LifeSnapshotStore::take() const
    {
        StateDump dump;

        dump.board = readBoard(world, grid);
        dump.dragging = drag.inProgress();

        // A run that registered no pointer sink has no drag to note.
        if (pointer.has_value())
        {
            const auto &sink = pointer->get();

            // Grid creates its entities row-major.
            // So an entity's distance from the first is its index.
            // The set orders ascending, so the list is deterministic.
            const auto first =
                antwika::ecs::rawValue(grid.entityAt(0, 0));

            for (const auto entity : sink.visitedCells())
            {
                const auto index =
                    antwika::ecs::rawValue(entity) - first;

                dump.visited.push_back(CellCoordinate{
                    .x = static_cast<std::uint32_t>(
                        index % grid.width()),
                    .y = static_cast<std::uint32_t>(
                        index / grid.width())});
            }

            dump.lastDrag = sink.lastDragPosition();
        }

        return dump;

        // gcov puts the returned value's unwind block here.
        // Nothing between the last push and the return throws.
    } // GCOVR_EXCL_LINE

    void LifeSnapshotStore::apply(const StateDump &dump)
    {
        // The board's size is in source rather than in the dump.
        // A dump of another size names a run this build never was.
        if (dump.board.width != grid.width()
            || dump.board.height != grid.height())
        {
            throw StateDumpError(
                "antwika::life: dump holds a "
                + std::to_string(dump.board.width) + "x"
                + std::to_string(dump.board.height)
                + " board where the running one is "
                + std::to_string(grid.width()) + "x"
                + std::to_string(grid.height()));
        }

        // Staged rather than switched.
        // The writes land at the next commit, as a toggle's do.
        for (std::uint32_t y = 0; y < grid.height(); ++y)
        {
            for (std::uint32_t x = 0; x < grid.width(); ++x)
            {
                const auto index =
                    std::size_t{y} * grid.width() + x;

                world.set<Cell>(
                    grid.entityAt(x, y),
                    Cell{.alive = dump.board.alive[index]});
            }
        }

        if (dump.dragging)
        {
            drag.begin();
        }
        else
        {
            drag.end();
        }

        // A run with no pointer sink drops the drag bookkeeping.
        // There is nothing that would read it.
        if (pointer.has_value())
        {
            std::set<antwika::ecs::Entity> visited;

            // In bounds by construction.
            // The decoder refused a coordinate off the dump's board.
            // And that board was just checked against the grid.
            for (const auto &cell : dump.visited)
            {
                visited.insert(grid.entityAt(cell.x, cell.y));
            }

            pointer->get().restoreDrag(
                std::move(visited), dump.lastDrag);
        }
    }

} // namespace antwika::life
