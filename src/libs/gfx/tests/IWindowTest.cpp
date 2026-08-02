#include <gtest/gtest.h>

#include <cstdlib>
#include <string>
#include <string_view>

#include "antwika/gfx/IRenderer.hpp"
#include "antwika/gfx/IWindow.hpp"
#include "antwika/gfx/Size.hpp"
#include "antwika/gfx/WindowId.hpp"

using antwika::gfx::IRenderer;
using antwika::gfx::IWindow;
using antwika::gfx::Size;
using antwika::gfx::WindowId;

namespace
{
    /**
     * @brief An IWindow that leaves configuredSize() alone.
     *
     * Deliberately not a mock and deliberately not in mocks/.
     * Overriding configuredSize() is exactly what must not happen here.
     * What is under test is the default the interface supplies.
     */
    class UnresizableWindow final : public IWindow
    {
    public:
        explicit UnresizableWindow(Size size)
            : reported(size)
        {
        }

        [[nodiscard]] WindowId id() const override
        {
            return WindowId{1};
        }

        [[nodiscard]] bool isOpen() const override
        {
            return true;
        }

        [[nodiscard]] std::string title() const override
        {
            return "Antwika";
        }

        [[nodiscard]] Size size() const override
        {
            return reported;
        }

        [[nodiscard]] bool isFullscreen() const override
        {
            return false;
        }

        [[nodiscard]] IRenderer &renderer() override
        {
            std::abort();
        }

        void setTitle(std::string_view /*title*/) override
        {
        }

        void setFullscreen(bool /*fullscreen*/) override
        {
        }

        void close() override
        {
        }

    private:
        Size reported;
    };
} // namespace

TEST(IWindowTest, ConfiguredSize_DefaultsToTheReportedSize)
{
    const UnresizableWindow window(Size{.width = 320, .height = 240});

    EXPECT_EQ(window.configuredSize(), window.size());
}

TEST(IWindowTest, ConfiguredSize_DefaultFollowsWhateverSizeReports)
{
    const UnresizableWindow small(Size{.width = 1, .height = 2});
    const UnresizableWindow large(Size{.width = 800, .height = 600});

    // The default is a forward, not a copy taken at construction.
    EXPECT_EQ(small.configuredSize(), (Size{.width = 1, .height = 2}));
    EXPECT_EQ(large.configuredSize(), (Size{.width = 800, .height = 600}));
}
