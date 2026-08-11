#include <gtest/gtest.h>

#include <cstdlib>
#include <string>
#include <string_view>

#include <antwika/gfx/fakes/FakeUnresizableWindow.hpp>

#include "antwika/gfx/IRenderer.hpp"
#include "antwika/gfx/IWindow.hpp"
#include "antwika/gfx/Size.hpp"
#include "antwika/gfx/WindowId.hpp"

using antwika::gfx::IRenderer;
using antwika::gfx::IWindow;
using antwika::gfx::Size;
using antwika::gfx::WindowId;
using antwika::gfx::fakes::FakeUnresizableWindow;

namespace
{
}

TEST(IWindowTest, ConfiguredSize_DefaultsToTheReportedSize)
{
    const FakeUnresizableWindow window(Size{.width = 320, .height = 240});

    ASSERT_EQ(window.size(), (Size{.width = 320, .height = 240}));
    EXPECT_EQ(window.configuredSize(), window.size());
}

TEST(IWindowTest, ConfiguredSize_DefaultFollowsWhateverSizeReports)
{
    const FakeUnresizableWindow small(Size{.width = 1, .height = 2});
    const FakeUnresizableWindow large(Size{.width = 800, .height = 600});

    EXPECT_EQ(small.configuredSize(), (Size{.width = 1, .height = 2}));
    EXPECT_EQ(large.configuredSize(), (Size{.width = 800, .height = 600}));
}

TEST(IWindowTest, SetSize_IsIgnoredByAWindowWithoutLiveResize)
{
    FakeUnresizableWindow window(Size{.width = 320, .height = 240});

    window.setSize(Size{.width = 800, .height = 600});

    EXPECT_EQ(window.size(), (Size{.width = 320, .height = 240}));
}
