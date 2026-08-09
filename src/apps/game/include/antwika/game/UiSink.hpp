#pragma once

#include <cstddef>

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/ui/Interactions.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/game/Camera.hpp"
#include "antwika/game/CityRatings.hpp"
#include "antwika/game/GameState.hpp"
#include "antwika/game/IMenuCommands.hpp"
#include "antwika/game/MapView.hpp"
#include "antwika/game/InputFold.hpp"
#include "antwika/game/MenuModalScene.hpp"
#include "antwika/game/PauseState.hpp"
#include "antwika/game/RoadDrag.hpp"
#include "antwika/game/Toolbar.hpp"
#include "antwika/game/UiOverlay.hpp"
#include "antwika/game/ViewCommands.hpp"

namespace antwika::game
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::ui::Interactions;
    using antwika::ui::Pointer;
    using antwika::ui::WidgetId;

    class UiSink final : public ITickEventSink
    {
    public:
        UiSink(
            const Camera &camera,
            UiOverlay &overlay,
            const InputFold &input,
            const Toolbar &toolbar,
            const PauseState &pause,
            MapViewState &view,
            IMenuCommands &commands,
            RoadDrag &drag,
            const MenuModalScene &modal,
            ViewCommands &viewCommands,
            const CityRatings &ratings,
            const GameState &state);

        UiSink(const UiSink &) = delete;
        UiSink(UiSink &&) = delete;

        UiSink &operator=(const UiSink &) = delete;
        UiSink &operator=(UiSink &&) = delete;

        void handle(const TickEvent &event) override;

        [[nodiscard]] bool menuOpen() const noexcept;

        [[nodiscard]] bool gameMenuOpen() const noexcept;

        [[nodiscard]] bool viewMenuOpen() const noexcept;

    private:
        [[nodiscard]] Pointer pointerNow(bool pressed) const;

        [[nodiscard]] Frame describeNow(bool pressed) const;

        void refreshAndAct(bool pressed);

        [[nodiscard]] bool actOnUi(
            const Interactions &interactions, bool pressed);

        void actOnBar(WidgetId activated);

        void actOnModal(WidgetId activated);

        void chooseFrom(std::size_t index);

        void chooseView(std::size_t index);

        void openModal();

        void selectFrom(WidgetId activated);

        const Camera &camera;
        UiOverlay &overlay;
        const InputFold &input;
        const Toolbar &toolbar;
        const PauseState &pause;
        MapViewState &view;
        IMenuCommands &commands;
        RoadDrag &drag;
        const MenuModalScene &modal;
        ViewCommands &viewCommands;
        const CityRatings &ratings;
        const GameState &state;

        bool modalOpen = false;

        bool listOpen = false;

        bool viewOpen = false;

        antwika::time::Tick tick = 0;
    };

}
