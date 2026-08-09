#include <chrono>
#include <iostream>
#include <memory>
#include <optional>
#include <string>

#include <antwika/app/RunRecorded.hpp>
#include <antwika/app/WindowedHost.hpp>
#include <antwika/gfx/SelectedBackend.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/input/SelectedInputBackend.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/time/SystemSleeper.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/app/TickLimitSource.hpp>

#include "antwika/ui_demo/DemoOverlay.hpp"
#include "antwika/ui_demo/DemoScene.hpp"
#include "antwika/ui_demo/DemoState.hpp"
#include "antwika/ui_demo/Messages.hpp"
#include "antwika/ui_demo/RenderSink.hpp"
#include "antwika/ui_demo/Showcase.hpp"
#include "antwika/ui_demo/UiDemo.hpp"

using antwika::app::RecordedRun;
using antwika::app::WindowedHost;
using antwika::app::WindowedSessionDesc;
using antwika::log::Level;
using antwika::time::SystemSleeper;
using antwika::ui_demo::DemoOverlay;
using antwika::ui_demo::DemoScene;
using antwika::ui_demo::DemoState;
using antwika::ui_demo::DemoSummary;
using antwika::ui_demo::Messages;
using antwika::ui_demo::RenderSink;
using antwika::ui_demo::showcaseNameId;
using antwika::app::TickLimitSource;

namespace
{
    constexpr antwika::gfx::Size kWindowSize{
        .width = 960, .height = 720};

    constexpr std::chrono::milliseconds kFramePeriod{40};

    constexpr antwika::time::Tick kTickBudget = 1500;

    void run(const RecordedRun &recorded)
    {
        WindowedHost host(
            std::cout,
            Level::Info,
            {.gfx = antwika::gfx::makeSelectedBackend,
             .input = antwika::input::makeSelectedInputBackend},
            WindowedSessionDesc{
                .name = "Antwika ui demo",
                .windowTitle = "Antwika ui demo",
                .canvas = kWindowSize,
                .input = {.coalescePointerMotion = true},
                .replayPath = recorded.options.replayPath});

        auto &logger = host.logger();
        auto &session = host.session();

        const antwika::ui_demo::Translator translator{
            antwika::i18n::kDefaultLocale};

        const DemoScene scene{translator};
        SystemSleeper sleeper;

        TickLimitSource source(
            session.source(), std::optional{kTickBudget});

        const DemoSummary summary = antwika::ui_demo::bootstrap({
            .logger = logger,
            .eventSink = recorded.eventSink,
            .inputSource = source,
            .codec = session.codec(),
            .translator = translator,
            .canvas = kWindowSize,
            .replayRecorder = recorded.replayRecorder,
            .extraSink =
                [&](const DemoState &, const DemoOverlay &overlay)
            {
                return std::make_unique<RenderSink>(
                    session.window(),
                    scene,
                    overlay,
                    sleeper,
                    kFramePeriod);
            }});

        logger.log(
            Level::Info,
            "Finished on the "
                + std::string{antwika::i18n::nameOf<Messages>(
                    showcaseNameId(summary.showcase))}
                + " page, having counted "
                + std::to_string(summary.clicks) + " clicks");
    }
}

int main(int argc, char **argv)
{
    return antwika::app::runRecorded(argc, argv, "antwika_ui_demo", run);
}
