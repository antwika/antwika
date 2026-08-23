#pragma once

#include <string>
#include <string_view>

#include "antwika/gfx/IRenderer.hpp"
#include "antwika/gfx/Size.hpp"
#include "antwika/gfx/WindowId.hpp"

namespace antwika::gfx
{

    class IWindow
    {
    public:
        virtual ~IWindow() = default;

        [[nodiscard]] virtual WindowId getId() const = 0;

        [[nodiscard]] virtual bool isOpen() const = 0;

        [[nodiscard]] virtual std::string getTitle() const = 0;

        [[nodiscard]] virtual Size getConfiguredSize() const
        {
            return getSize();
        }

        [[nodiscard]] virtual Size getSize() const = 0;

        [[nodiscard]] virtual bool isFullscreen() const = 0;

        [[nodiscard]] virtual IRenderer &renderer() = 0;

        virtual void setTitle(std::string_view title) = 0;

        virtual void setSize(Size size) = 0;

        virtual void setFullscreen(bool fullscreen) = 0;

        virtual void close() = 0;
    };

}
