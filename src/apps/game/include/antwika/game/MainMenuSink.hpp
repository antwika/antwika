#pragma once

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/ui/Pointer.hpp>

#include "antwika/game/AppMode.hpp"
#include "antwika/game/InputFold.hpp"
#include "antwika/game/LocaleState.hpp"
#include "antwika/game/MainMenuScene.hpp"
#include "antwika/game/OptionsScene.hpp"
#include "antwika/game/OptionsState.hpp"
#include "antwika/game/UiOverlay.hpp"

namespace antwika::game
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::ui::Pointer;

    class MainMenuSink final : public ITickEventSink
    {
    public:
        MainMenuSink(
            AppModeState &mode,
            UiOverlay &overlay,
            const InputFold &input,
            const MainMenuScene &scene,
            ITickEventSink &stop,
            OptionsState &options,
            const OptionsScene &optionsScene,
            LocaleState &locale);

        MainMenuSink(const MainMenuSink &) = delete;
        MainMenuSink(MainMenuSink &&) = delete;

        MainMenuSink &operator=(const MainMenuSink &) = delete;
        MainMenuSink &operator=(MainMenuSink &&) = delete;

        void handle(const TickEvent &event) override;

    private:
        [[nodiscard]] Pointer pointerNow(bool pressed) const;

        void refreshAndAct(const TickEvent &event, bool pressed);

        void refreshOptions(bool pressed);

        AppModeState &mode;
        UiOverlay &overlay;
        const InputFold &input;
        const MainMenuScene &scene;
        ITickEventSink &stop;
        OptionsState &options;
        const OptionsScene &optionsScene;
        LocaleState &locale;
    };

}
