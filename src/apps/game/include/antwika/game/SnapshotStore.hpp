#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include <antwika/console/JsonSnapshotStore.hpp>

#include "antwika/game/LocaleState.hpp"
#include "antwika/game/MapView.hpp"
#include "antwika/game/PauseState.hpp"
#include "antwika/game/SaveFormatError.hpp"
#include "antwika/game/SessionStore.hpp"
#include "antwika/game/StateDump.hpp"
#include "antwika/game/UiOverlay.hpp"

namespace antwika::game
{

    /**
     * @brief This application's half of dump_state and load_state.
     *
     * console::SnapshotCommands owns the policy -- the messages, the
     * refusal while recording or replaying, the history mechanics --
     * and this owns what the state *is*: a StateDump taken from and
     * applied to the live session, carried as the opaque state object
     * of the shared envelope under this application's own magic,
     * version and migrations.
     *
     * A load restores through the very SessionStore the picker uses,
     * stages the language change to the tick boundary through
     * LocaleState, and puts the palette and the map view back exactly
     * as the dump held them.
     */
    class GameSnapshotStore final
        : public antwika::console::JsonSnapshotStore<SaveFormatError>
    {
    public:
        /**
         * @brief Construct the store over the live session.
         * @param session Taken from by a dump, put back by a load.
         * Must outlive this store.
         * @param pause Carried by a dump, restored by a load. Must
         * outlive this store.
         * @param toolbar The toolbar's shared state, for the selected
         * tool. Must outlive this store.
         * @param view Which picture of the city is showing. Must
         * outlive this store.
         * @param locale The run's language, staged back by a load.
         * Must outlive this store.
         */
        GameSnapshotStore(
            SessionStore &session,
            PauseState &pause,
            UiOverlay &toolbar,
            MapViewState &view,
            LocaleState &locale) noexcept;

        GameSnapshotStore(const GameSnapshotStore &) = delete;
        GameSnapshotStore(GameSnapshotStore &&) = delete;

        GameSnapshotStore &operator=(const GameSnapshotStore &) = delete;
        GameSnapshotStore &operator=(GameSnapshotStore &&) = delete;

    private:
        [[nodiscard]] nlohmann::json takeState(
            const std::string &path) override;

        void applyState(
            const std::string &path,
            const nlohmann::json &state) override;

        [[nodiscard]] StateDump take() const;

        void apply(const StateDump &dump);

        SessionStore &session;
        PauseState &pause;
        UiOverlay &toolbar;
        MapViewState &view;
        LocaleState &locale;
    };

} // namespace antwika::game
