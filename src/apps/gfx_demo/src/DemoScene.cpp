#include "antwika/gfx_demo/DemoScene.hpp"

#include <array>
#include <cstdint>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Rect.hpp>

namespace antwika::gfx_demo
{

    using antwika::gfx::Color;
    using antwika::gfx::Rect;

    namespace
    {
        constexpr std::uint32_t kBarCount = 3;

        constexpr Color kBackground{.red = 16, .green = 16, .blue = 24};

        constexpr std::array<Color, kBarCount> kBarColors{
            Color{.red = 224, .green = 64, .blue = 64},
            Color{.red = 64, .green = 224, .blue = 96},
            Color{.red = 80, .green = 128, .blue = 240},
        };

        // Bars and gaps are all one unit wide, with a gap at each end.
        // That keeps the row centred whatever the canvas size is.
        constexpr std::uint32_t kUnitsAcross = kBarCount * 2 + 1;
    } // namespace

    void DemoScene::draw(IRenderer &renderer, Size canvas) const
    {
        renderer.clear(kBackground);

        const std::uint32_t unit = canvas.width / kUnitsAcross;
        const std::uint32_t barHeight = canvas.height / 2;
        const auto top = static_cast<std::int32_t>(canvas.height / 4);

        for (std::uint32_t index = 0; index < kBarCount; ++index)
        {
            const auto left = static_cast<std::int32_t>(unit * (index * 2 + 1));

            renderer.drawRect(
                Rect{
                    .origin = {.x = left, .y = top},
                    .size = {.width = unit, .height = barHeight}},
                kBarColors.at(index));
        }
    }

} // namespace antwika::gfx_demo
