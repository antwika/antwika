#include "antwika/game/SnapshotStore.hpp"

namespace antwika::game
{

    GameSnapshotStore::GameSnapshotStore(
        SessionStore &session,
        PauseState &pause,
        UiOverlay &toolbar,
        MapViewState &view,
        LocaleState &locale) noexcept
        : antwika::console::JsonSnapshotStore<SaveFormatError>(
              {.magic = kStateDumpMagic,
               .version = kStateDumpVersion},
              "antwika game state dump document",
              standardStateDumpMigrations),
          session(session),
          pause(pause),
          toolbar(toolbar),
          view(view),
          locale(locale)
    {
    }

    nlohmann::json GameSnapshotStore::takeState(const std::string &)
    {
        return stateDumpToJson(take());
    }

    void GameSnapshotStore::applyState(
        const std::string &, const nlohmann::json &state)
    {
        apply(stateDumpFromJson(state));
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
