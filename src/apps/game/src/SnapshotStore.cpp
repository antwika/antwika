#include "antwika/game/SnapshotStore.hpp"

#include <antwika/console/SnapshotFormat.hpp>

#include "antwika/game/SaveFormatError.hpp"

namespace antwika::game
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
                "antwika game state dump document",
                standardStateDumpMigrations); // GCOVR_EXCL_LINE
            return format;
        }
    } // namespace

    GameSnapshotStore::GameSnapshotStore(
        SessionStore &session,
        PauseState &pause,
        UiOverlay &toolbar,
        MapViewState &view,
        LocaleState &locale) noexcept
        : session(session),
          pause(pause),
          toolbar(toolbar),
          view(view),
          locale(locale)
    {
    }

    void GameSnapshotStore::dump(
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

    std::vector<std::string> GameSnapshotStore::load(
        const std::string &path)
    {
        auto snapshot = dumpFormat().read(path);

        try
        {
            apply(stateDumpFromJson(snapshot.state));
        }
        // The state's own reader promises SaveFormatError.
        // What this seam promises is console::SnapshotError.
        // So it is rewrapped here, exactly as keyNamed() rewraps.
        catch (const SaveFormatError &failed) // GCOVR_EXCL_LINE
        {
            throw antwika::console::SnapshotError(failed.what());
        }

        return snapshot.console;
    }

    StateDump GameSnapshotStore::take() const
    {
        StateDump dump;

        dump.save = session.take();
        dump.paused = pause.paused();
        dump.tool = toolbar.tool();
        dump.view = view.view();
        dump.locale = locale.locale();

        return dump;

        // gcov puts the returned value's unwind block here.
        // SaveGame.cpp's own encoder explains it at length.
        // No input reaches it.
    } // GCOVR_EXCL_LINE

    void GameSnapshotStore::apply(const StateDump &dump)
    {
        session.restore(dump.save);
        pause.set(dump.paused);

        if (dump.tool.has_value())
        {
            toolbar.select(*dump.tool);
        }
        else
        {
            toolbar.clearTool();
        }

        view.set(dump.view);

        // Staged rather than switched.
        // So it lands at the tick boundary, as the options screen's.
        locale.request(dump.locale);
    }

} // namespace antwika::game
