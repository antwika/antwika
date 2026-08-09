#pragma once

#include <optional>

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/input/IInputEventCodec.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputState.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::console
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::gfx::Point;
    using antwika::input::IInputEventCodec;
    using antwika::input::InputEvent;
    using antwika::input::InputState;

    class InputFold final : public ITickEventSink
    {
    public:
        explicit InputFold(const IInputEventCodec &codec);

        InputFold(const InputFold &) = delete;
        InputFold(InputFold &&) = delete;

        InputFold &operator=(const InputFold &) = delete;
        InputFold &operator=(InputFold &&) = delete;

        void handle(const TickEvent &event) override;

        [[nodiscard]] const std::optional<InputEvent> &
        current() const noexcept;

        [[nodiscard]] const InputState &state() const noexcept;

        [[nodiscard]] Point pointer() const noexcept;

        [[nodiscard]] Point pointerBefore() const noexcept;

        [[nodiscard]] bool located() const noexcept;

    private:
        const IInputEventCodec &codec;

        InputState folded;
        std::optional<InputEvent> latest;
        std::optional<antwika::time::Tick> foldedTick;

        Point previous{};
        bool hasPosition = false;
    };

}
