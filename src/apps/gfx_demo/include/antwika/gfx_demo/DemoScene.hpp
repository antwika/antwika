#pragma once

#include <cstdint>

#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/ui/DrawList.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/WidgetId.hpp>

namespace antwika::gfx_demo
{

    using antwika::gfx::IRenderer;
    using antwika::gfx::ITexture;
    using antwika::gfx::Size;
    using antwika::ui::DrawList;
    using antwika::ui::Frame;
    using antwika::ui::Pointer;
    using antwika::ui::WidgetId;

    namespace widgets
    {
        inline constexpr WidgetId kCount{1};

        inline constexpr WidgetId kReset{2};
    }

    class DemoScene final
    {
    public:
        void draw(
            IRenderer &renderer,
            Size canvas,
            const ITexture &logo,
            const DrawList &overlay) const;

        [[nodiscard]] Frame describe(
            Size canvas,
            Pointer pointer = {},
            std::uint32_t clicks = 0) const;
    };

}
