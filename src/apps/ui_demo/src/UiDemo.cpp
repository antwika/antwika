#include "antwika/ui_demo/UiDemo.hpp"

#include <memory>
#include <vector>

#include <antwika/engine/Engine.hpp>
#include <antwika/engine/StopSignal.hpp>
#include <antwika/event/EventDispatcher.hpp>
#include <antwika/event/TickedEventDispatcher.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/simulation/EngineLoop.hpp>

#include "antwika/ui_demo/DemoScene.hpp"
#include "antwika/ui_demo/DemoSink.hpp"

namespace antwika::ui_demo
{

    using antwika::engine::Engine;
    using antwika::engine::StopSignal;
    using antwika::event::EventDispatcher;
    using antwika::event::TickedEventDispatcher;
    using antwika::simulation::EngineLoop;

    DemoSummary bootstrap(const UiDemoConfig &config)
    {
        ILogger &logger = config.logger;

        DemoState state;
        DemoOverlay overlay(config.canvas);
        const DemoScene scene;

        EventDispatcher dispatcher({config.eventSink});

        // The order is the whole wiring.
        // The showcase resolves this tick's input and describes itself.
        // Then whatever draws it, so a frame is of the finished tick.
        DemoSink demo(state, overlay, config.codec, scene);
        StopSignal stopSignal;

        std::vector<std::reference_wrapper<ITickEventSink>> timedSinks{
            demo, stopSignal};

        // Held out here rather than inside the if.
        // The sink has to outlive the reference the dispatcher keeps.
        std::unique_ptr<ITickEventSink> extra;
        if (config.extraSink)
        {
            extra = config.extraSink(state, overlay);
            timedSinks.push_back(*extra);
        }

        if (config.replayRecorder.has_value())
        {
            timedSinks.push_back(config.replayRecorder->get());
        }

        TickedEventDispatcher tickedDispatcher(dispatcher, timedSinks);
        Engine engine(logger, tickedDispatcher);

        logger.log(
            antwika::log::Level::Info, "Running the antwika::ui demo");
        engine.start();

        EngineLoop loop(engine, tickedDispatcher, config.inputSource);
        loop.run(stopSignal, config.maxTicks);

        return DemoSummary{
            .showcase = state.showcase(),
            .clicks = state.clicks(),
            .commands = overlay.commands().size()};
    }

} // namespace antwika::ui_demo
