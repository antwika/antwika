#pragma once

#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/IPointerMapping.hpp>
#include <antwika/input/Position.hpp>

namespace antwika::app
{

    using antwika::gfx::IWindow;
    using antwika::gfx::Size;
    using antwika::input::IPointerMapping;
    using antwika::input::Position;

    class WindowPointerMapping final : public IPointerMapping
    {
    public:
        WindowPointerMapping(const IWindow &window, Size canvasSize);

        WindowPointerMapping(const WindowPointerMapping &) = delete;
        WindowPointerMapping(WindowPointerMapping &&) = delete;

        WindowPointerMapping &operator=(
            const WindowPointerMapping &) = delete;
        WindowPointerMapping &operator=(WindowPointerMapping &&) = delete;

        [[nodiscard]] Position toCanvas(
            Position position) const override;

    private:
        const IWindow &window;
        Size canvasSize;
    };

}
