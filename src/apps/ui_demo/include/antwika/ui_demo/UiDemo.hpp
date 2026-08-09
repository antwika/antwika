#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>

#include <antwika/event/IEventSink.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/IInputEventCodec.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/ui_demo/DemoOverlay.hpp"
#include "antwika/ui_demo/DemoState.hpp"
#include "antwika/ui_demo/Messages.hpp"
#include "antwika/ui_demo/Showcase.hpp"

namespace antwika::ui_demo
{

    using antwika::event::IEventSink;
    using antwika::event::ITickEventSink;
    using antwika::gfx::Size;
    using antwika::input::IInputEventCodec;
    using antwika::log::ILogger;
    using antwika::event::ITickEventSource;

    struct DemoSummary final
    {
        Showcase showcase = Showcase::Labels;

        std::uint32_t clicks = 0;

        std::size_t commands = 0;
    };

    using TickSinkFactory = std::function<
        std::unique_ptr<ITickEventSink>(const DemoState &,
                                        const DemoOverlay &)>;

    struct UiDemoConfig final
    {
        ILogger &logger;

        IEventSink &eventSink;

        ITickEventSource &inputSource;

        const IInputEventCodec &codec;

        const Translator &translator;

        Size canvas;

        std::optional<antwika::time::Tick> maxTicks = std::nullopt;

        std::optional<std::reference_wrapper<ITickEventSink>>
            replayRecorder = std::nullopt;

        TickSinkFactory extraSink = {};
    };

    DemoSummary bootstrap(const UiDemoConfig &config);

}
