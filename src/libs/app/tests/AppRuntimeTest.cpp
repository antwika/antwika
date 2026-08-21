#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <sstream>
#include <stdexcept>

#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/WindowSpec.hpp>
#include <antwika/gfx/WindowId.hpp>
#include <antwika/gfx/mocks/MockGfxBackend.hpp>
#include <antwika/gfx/mocks/MockWindow.hpp>
#include <antwika/input/fakes/FakeInputBackend.hpp>
#include <antwika/log/Level.hpp>

#include "antwika/app/AppRuntime.hpp"

using antwika::app::BackendFactories;
using antwika::app::AppRuntime;
using antwika::app::WindowedSessionSpec;
using antwika::gfx::IGfxBackend;
using antwika::gfx::Size;
using antwika::gfx::WindowSpec;
using antwika::gfx::WindowId;
using antwika::gfx::mocks::MockGfxBackend;
using antwika::gfx::mocks::MockWindow;
using antwika::input::IInputBackend;
using antwika::input::fakes::FakeInputBackend;
using antwika::log::ILogger;
using antwika::log::Level;
using ::testing::NiceMock;
using ::testing::Return;

namespace
{
    constexpr Size kCanvasSize{.width = 640, .height = 480};

    constexpr WindowId kWindow{3};

    [[nodiscard]] std::unique_ptr<IGfxBackend> aGfxBackend(ILogger &)
    {
        auto backend = std::make_unique<NiceMock<MockGfxBackend>>();

        ON_CALL(*backend, createWindow)
            .WillByDefault(
                [](const WindowSpec &)
                {
                    auto window =
                        std::make_unique<NiceMock<MockWindow>>();

                    ON_CALL(*window, id).WillByDefault(Return(kWindow));
                    ON_CALL(*window, size).WillByDefault(Return(kCanvasSize));

                    return window;
                });

        return backend;
    }

    [[nodiscard]] std::unique_ptr<IInputBackend> anInputBackend(ILogger &)
    {
        return std::make_unique<FakeInputBackend>();
    }

    [[nodiscard]] BackendFactories working()
    {
        return BackendFactories{
            .gfx = aGfxBackend, .input = anInputBackend};
    }

    [[nodiscard]] WindowedSessionSpec aSpec()
    {
        return WindowedSessionSpec{
            .name = "host", .windowTitle = "Host", .canvasSize = kCanvasSize};
    }
}

TEST(AppRuntimeTest, Ctor_OpensASessionOnTheMadeBackends)
{
    std::ostringstream outputStream;

    AppRuntime hostRuntime(outputStream, Level::Info, working(), aSpec());

    EXPECT_EQ(hostRuntime.session().canvas(), kCanvasSize);
}

TEST(AppRuntimeTest, Logger_WritesToTheStreamItWasGiven)
{
    std::ostringstream outputStream;

    AppRuntime hostRuntime(outputStream, Level::Info, working(), aSpec());

    hostRuntime.logger().log(Level::Info, "hello");

    EXPECT_THAT(outputStream.str(), ::testing::HasSubstr("hello"));
}

TEST(AppRuntimeTest, Logger_KeepsBackWhatIsBelowTheMinimum)
{
    std::ostringstream outputStream;

    AppRuntime hostRuntime(outputStream, Level::Warning, working(), aSpec());

    hostRuntime.logger().log(Level::Info, "hello");

    EXPECT_EQ(outputStream.str(), "");
}

TEST(AppRuntimeTest, Ctor_RefusesAnAbsentGraphicsFactory)
{
    std::ostringstream outputStream;

    EXPECT_THROW(
        AppRuntime(
            outputStream,
            Level::Info,
            BackendFactories{.gfx = {}, .input = anInputBackend},
            aSpec()),
        std::invalid_argument);
}

TEST(AppRuntimeTest, Ctor_RefusesAnAbsentInputFactory)
{
    std::ostringstream outputStream;

    EXPECT_THROW(
        AppRuntime(
            outputStream,
            Level::Info,
            BackendFactories{.gfx = aGfxBackend, .input = {}},
            aSpec()),
        std::invalid_argument);
}

TEST(AppRuntimeTest, Ctor_RefusesAGraphicsFactoryThatMakesNothing)
{
    std::ostringstream outputStream;

    EXPECT_THROW(
        AppRuntime(
            outputStream,
            Level::Info,
            BackendFactories{
                .gfx = [](ILogger &) { return nullptr; },
                .input = anInputBackend},
            aSpec()),
        std::invalid_argument);
}

TEST(AppRuntimeTest, Ctor_RefusesAnInputFactoryThatMakesNothing)
{
    std::ostringstream outputStream;

    EXPECT_THROW(
        AppRuntime(
            outputStream,
            Level::Info,
            BackendFactories{
                .gfx = aGfxBackend,
                .input = [](ILogger &) { return nullptr; }},
            aSpec()),
        std::invalid_argument);
}
