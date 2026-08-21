#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/input/IFramePump.hpp>
#include <antwika/time/IClock.hpp>
#include <antwika/time/ISleeper.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/app/FramePacing.hpp"
#include "antwika/app/IFramePacingSink.hpp"
#include "antwika/app/IFramePass.hpp"

namespace antwika::app
{

    using antwika::event::Event;
    using antwika::event::ITickEventSource;
    using antwika::time::ISleeper;

    class FramePacedSource final : public ITickEventSource
    {
    public:
        FramePacedSource(
            ITickEventSource &innerSource,
            IFramePass &pass,
            ISleeper &sleeper,
            const antwika::time::IClock &clock,
            FramePacing pacing,
            std::optional<std::reference_wrapper<antwika::input::IFramePump>>
                pump = std::nullopt,
            std::optional<std::reference_wrapper<IFramePacingSink>>
                pacingSink = std::nullopt);

        FramePacedSource(const FramePacedSource &) = delete;
        FramePacedSource(FramePacedSource &&) = delete;

        FramePacedSource &operator=(const FramePacedSource &) = delete;
        FramePacedSource &operator=(FramePacedSource &&) = delete;

        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

    private:
        [[nodiscard]] bool waitUntil(
            std::chrono::time_point<std::chrono::system_clock> startTime,
            std::chrono::microseconds elapsedTime);

        void sample(antwika::time::Tick tick);

        ITickEventSource &innerSource;
        IFramePass &pass;
        ISleeper &sleeper;
        const antwika::time::IClock &clock;
        FramePacing pacing;
        std::optional<std::reference_wrapper<antwika::input::IFramePump>>
            pump;
        std::optional<std::reference_wrapper<IFramePacingSink>>
            pacingSink;
    };

}
