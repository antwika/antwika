#pragma once

#include <string>

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/ui/Interactions.hpp>
#include <antwika/ui/Keyboard.hpp>
#include <antwika/ui/Pointer.hpp>

#include "antwika/game/AppMode.hpp"
#include "antwika/game/InputFold.hpp"
#include "antwika/game/OptionsState.hpp"
#include "antwika/game/SaveLoadScene.hpp"
#include "antwika/game/SaveLoadState.hpp"
#include "antwika/game/SessionStore.hpp"
#include "antwika/game/UiOverlay.hpp"

namespace antwika::game
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::ui::Interactions;
    using antwika::ui::Keyboard;
    using antwika::ui::Pointer;

    class SaveLoadSink final : public ITickEventSink
    {
    public:
        SaveLoadSink(
            SaveLoadState &state,
            AppModeState &mode,
            UiOverlay &overlay,
            const InputFold &input,
            const SaveLoadScene &scene,
            SessionStore &session,
            const OptionsState &options,
            std::string directory);

        SaveLoadSink(const SaveLoadSink &) = delete;
        SaveLoadSink(SaveLoadSink &&) = delete;

        SaveLoadSink &operator=(const SaveLoadSink &) = delete;
        SaveLoadSink &operator=(SaveLoadSink &&) = delete;

        void handle(const TickEvent &event) override;

    private:
        [[nodiscard]] Pointer pointerNow(bool pressed) const;

        void refreshAndAct(bool pressed, const Keyboard &keyboard);

        void act(const Interactions &interactions);

        void saveNow();

        void loadNow();

        SaveLoadState &state;
        AppModeState &mode;
        UiOverlay &overlay;
        const InputFold &input;
        const SaveLoadScene &scene;
        SessionStore &session;
        const OptionsState &options;
        std::string directory;
    };

}
