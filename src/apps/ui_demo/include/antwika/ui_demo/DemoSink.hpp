#pragma once

#include <optional>

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/input/IInputEventCodec.hpp>
#include <antwika/input/InputState.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/ui/Interactions.hpp>
#include <antwika/ui/Keyboard.hpp>
#include <antwika/ui/OptionChoice.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/TextEdit.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/ui_demo/DemoOverlay.hpp"
#include "antwika/ui_demo/DemoScene.hpp"
#include "antwika/ui_demo/DemoState.hpp"

namespace antwika::ui_demo
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::input::IInputEventCodec;
    using antwika::input::InputState;
    using antwika::ui::Interactions;
    using antwika::ui::OptionChoice;
    using antwika::ui::TextEdit;
    using antwika::ui::WidgetId;

    class DemoSink final : public ITickEventSink
    {
    public:
        DemoSink(
            DemoState &state,
            DemoOverlay &overlay,
            const IInputEventCodec &codec,
            const DemoScene &scene);

        DemoSink(const DemoSink &) = delete;
        DemoSink(DemoSink &&) = delete;

        DemoSink &operator=(const DemoSink &) = delete;
        DemoSink &operator=(DemoSink &&) = delete;

        void handle(const TickEvent &event) override;

    private:
        [[nodiscard]] Pointer pointerNow(
            bool pressed, bool extends) const;

        void refreshAndAct(
            bool pressed, bool extends, const Keyboard &keyboard);

        void act(const Interactions &interactions);

        void choose(const OptionChoice &choice);

        void edit(const TextEdit &change);

        void press(WidgetId activated);

        DemoState &state;
        DemoOverlay &overlay;
        const IInputEventCodec &codec;
        const DemoScene &scene;

        InputState folded;
        std::optional<antwika::time::Tick> foldedTick;
        bool located = false;
    };

}
