#pragma once

#include <nlohmann/json_fwd.hpp>

#include <string>

#include <antwika/console/IJsonSnapshotStore.hpp>

#include "antwika/game/LocaleState.hpp"
#include "antwika/game/MapView.hpp"
#include "antwika/game/PauseState.hpp"
#include "antwika/game/SaveFormatError.hpp"
#include "antwika/game/SessionStore.hpp"
#include "antwika/game/StateDump.hpp"
#include "antwika/game/UiOverlay.hpp"

namespace antwika::game
{

    class GameSnapshotStore final
        : public antwika::console::IJsonSnapshotStore<SaveFormatError>
    {
    public:
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

}
