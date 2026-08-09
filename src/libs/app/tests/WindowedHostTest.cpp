#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <sstream>
#include <stdexcept>

#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/gfx/WindowId.hpp>
#include <antwika/gfx/mocks/MockGfxBackend.hpp>
#include <antwika/gfx/mocks/MockWindow.hpp>
#include <antwika/input/fakes/FakeInputBackend.hpp>
#include <antwika/log/Level.hpp>

#include "antwika/app/WindowedHost.hpp"

using antwika::app::BackendFactories;
using antwika::app::WindowedHost;
using antwika::app::WindowedSessionDesc;
using antwika::gfx::IGfxBackend;
using antwika::gfx::Size;
using antwika::gfx::WindowDesc;
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
    constexpr Size kCanvas{.width = 640, .height = 480};

    constexpr WindowId kWindow{3};

    [[nodiscard]] std::unique_ptr<IGfxBackend> aGfxBackend(ILogger &)
    {
        auto backend = std::make_unique<NiceMock<MockGfxBackend>>();

        ON_CALL(*backend, createWindow)
            .WillByDefault(
                [](const WindowDesc &)
                {
                    auto window =
                        std::make_unique<NiceMock<MockWindow>>();

                    ON_CALL(*window, id).WillByDefault(Return(kWindow));
                    ON_CALL(*window, size).WillByDefault(Return(kCanvas));

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

    [[nodiscard]] WindowedSessionDesc aDesc()
    {
        return WindowedSessionDesc{
            .name = "host", .windowTitle = "Host", .canvas = kCanvas};
    }
}

TEST(WindowedHostTest, Ctor_OpensASessionOnTheMadeBackends)
{
    std::ostringstream out;

    WindowedHost host(out, Level::Info, working(), aDesc());

    EXPECT_EQ(host.session().canvas(), kCanvas);
}

TEST(WindowedHostTest, Logger_WritesToTheStreamItWasGiven)
{
    std::ostringstream out;

    WindowedHost host(out, Level::Info, working(), aDesc());

    host.logger().log(Level::Info, "hello");

    EXPECT_THAT(out.str(), ::testing::HasSubstr("hello"));
}

TEST(WindowedHostTest, Logger_KeepsBackWhatIsBelowTheMinimum)
{
    std::ostringstream out;

    WindowedHost host(out, Level::Warning, working(), aDesc());

    host.logger().log(Level::Info, "hello");

    EXPECT_EQ(out.str(), "");
}

TEST(WindowedHostTest, Ctor_RefusesAnAbsentGraphicsFactory)
{
    std::ostringstream out;

    EXPECT_THROW(
        WindowedHost(
            out,
            Level::Info,
            BackendFactories{.gfx = {}, .input = anInputBackend},
            aDesc()),
        std::invalid_argument);
}

TEST(WindowedHostTest, Ctor_RefusesAnAbsentInputFactory)
{
    std::ostringstream out;

    EXPECT_THROW(
        WindowedHost(
            out,
            Level::Info,
            BackendFactories{.gfx = aGfxBackend, .input = {}},
            aDesc()),
        std::invalid_argument);
}

TEST(WindowedHostTest, Ctor_RefusesAGraphicsFactoryThatMakesNothing)
{
    std::ostringstream out;

    EXPECT_THROW(
        WindowedHost(
            out,
            Level::Info,
            BackendFactories{
                .gfx = [](ILogger &) { return nullptr; },
                .input = anInputBackend},
            aDesc()),
        std::invalid_argument);
}

TEST(WindowedHostTest, Ctor_RefusesAnInputFactoryThatMakesNothing)
{
    std::ostringstream out;

    EXPECT_THROW(
        WindowedHost(
            out,
            Level::Info,
            BackendFactories{
                .gfx = aGfxBackend,
                .input = [](ILogger &) { return nullptr; }},
            aDesc()),
        std::invalid_argument);
}
