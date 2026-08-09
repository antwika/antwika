#pragma once

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>

#include "antwika/game/InputFold.hpp"
#include "antwika/game/OptionsState.hpp"
#include "antwika/game/ViewCommands.hpp"

namespace antwika::game
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;

    class HotkeySink final : public ITickEventSink
    {
    public:

        HotkeySink(
            const OptionsState &options,
            const InputFold &input,
            ViewCommands &view) noexcept;

        HotkeySink(const HotkeySink &) = delete;
        HotkeySink(HotkeySink &&) = delete;

        HotkeySink &operator=(const HotkeySink &) = delete;
        HotkeySink &operator=(HotkeySink &&) = delete;

        void handle(const TickEvent &event) override;

    private:
        void act(Action action) noexcept;

        const OptionsState &options;
        const InputFold &input;
        ViewCommands &view;
    };

}
