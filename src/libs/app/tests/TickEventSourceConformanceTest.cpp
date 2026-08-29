#include <gmock/gmock.h>

#include <chrono>
#include <memory>
#include <optional>

#include <antwika/app/fakes/FakeFramePass.hpp>
#include <antwika/event/conformance/ScriptedSourceTraits.hpp>
#include <antwika/event/conformance/TickEventSourceConformanceTest.hpp>
#include <antwika/gfx/NullBackend.hpp>
#include <antwika/gfx/WindowSpec.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/time/fakes/FakeClock.hpp>
#include <antwika/time/fakes/FakeSleeper.hpp>

#include "antwika/app/FramePacedSource.hpp"
#include "antwika/app/FramePacing.hpp"
#include "antwika/app/FullscreenToggleSource.hpp"
#include "antwika/app/TickLimitSource.hpp"
#include "antwika/app/WindowCloseSource.hpp"
#include "antwika/app/WindowInputSource.hpp"

namespace antwika::event::conformance
{

    namespace
    {

        using antwika::log::mocks::MockLogger;
        using ::testing::NiceMock;

        [[nodiscard]] gfx::WindowSpec getConformanceSpec()
        {
            return gfx::WindowSpec{
                .title = "tick source conformance",
                .size = {.width = 64, .height = 64}};
        }

        struct WindowDependencies
        {
            NiceMock<MockLogger> logger;
            gfx::NullBackend backend{logger};
            std::unique_ptr<gfx::IWindow> window{
                backend.createWindow(getConformanceSpec())};
        };

        struct FramePacedDependencies
        {
            app::fakes::FakeFramePass pass;
            time::fakes::FakeClock clock{
                std::chrono::time_point<std::chrono::system_clock>{}};
            time::fakes::FakeSleeper sleeper;
        };

        struct FullscreenToggleDependencies : WindowDependencies
        {
            input::InputEventCodec codec;
        };

        struct TickLimitSourceTraits final
            : ScriptedSourceTraits<app::TickLimitSource>
        {
            TickLimitSourceTraits()
                : ScriptedSourceTraits(std::nullopt)
            {
            }
        };

        struct FramePacedSourceTraits final
            : FramePacedDependencies,
              ScriptedSourceTraits<app::FramePacedSource>
        {
            FramePacedSourceTraits()
                : ScriptedSourceTraits(
                      pass,
                      sleeper,
                      clock,
                      app::FramePacing{
                          .tickInterval = std::chrono::milliseconds{0},
                          .framesPerTick = 1})
            {
            }
        };

        struct WindowCloseSourceTraits final
            : WindowDependencies,
              ScriptedSourceTraits<app::WindowCloseSource>
        {
            WindowCloseSourceTraits()
                : ScriptedSourceTraits(backend, *window)
            {
            }
        };

        struct WindowInputSourceTraits final
            : WindowDependencies,
              ScriptedSourceTraits<app::WindowInputSource>
        {
            WindowInputSourceTraits()
                : ScriptedSourceTraits(backend, window->getId())
            {
            }
        };

        struct FullscreenToggleSourceTraits final
            : FullscreenToggleDependencies,
              ScriptedSourceTraits<app::FullscreenToggleSource>
        {
            FullscreenToggleSourceTraits()
                : ScriptedSourceTraits(*window, codec, input::Key::F10)
            {
            }
        };

    }

    INSTANTIATE_TYPED_TEST_SUITE_P(
        TickLimit, TickEventSourceConformanceTest, TickLimitSourceTraits);

    INSTANTIATE_TYPED_TEST_SUITE_P(
        FramePaced, TickEventSourceConformanceTest, FramePacedSourceTraits);

    INSTANTIATE_TYPED_TEST_SUITE_P(
        WindowClose, TickEventSourceConformanceTest, WindowCloseSourceTraits);

    INSTANTIATE_TYPED_TEST_SUITE_P(
        WindowInput, TickEventSourceConformanceTest, WindowInputSourceTraits);

    INSTANTIATE_TYPED_TEST_SUITE_P(
        FullscreenToggle,
        TickEventSourceConformanceTest,
        FullscreenToggleSourceTraits);

}
