#pragma once

#include <cstdlib>
#include <string>
#include <string_view>

#include "antwika/gfx/IRenderer.hpp"
#include "antwika/gfx/IWindow.hpp"
#include "antwika/gfx/Size.hpp"
#include "antwika/gfx/WindowId.hpp"

namespace antwika::gfx::fakes
{

    class FakeUnresizableWindow final : public IWindow
    {
    public:
        explicit FakeUnresizableWindow(Size size)
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

        void setTitle(std::string_view ) override
        {
        }

        void setFullscreen(bool ) override
        {
        }

        void close() override
        {
        }

    private:
        Size reported;
    };

}
