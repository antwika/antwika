#include "antwika/ui/Painter.hpp"

#include <variant>

#include <antwika/gfx/RectF.hpp>

#include "antwika/ui/DrawCommand.hpp"

namespace antwika::ui
{

    void paint(IRenderer &renderer, const DrawList &drawList)
    {
        for (const auto &command : drawList)
        {
            if (const auto *fill = std::get_if<FillRect>(&command))
            {
                renderer.drawRect(fill->rect, fill->color);

                continue;
            }

            if (const auto *picture =
                    std::get_if<DrawTexture>(&command))
            {
                if (picture->texture != nullptr)
                {
                    renderer.drawTexture(
                        *picture->texture,
                        picture->sourceRect,
                        picture->destinationRect,
                        picture->tintColor);
                }

                continue;
            }

            if (const auto *begun =
                    std::get_if<PushClip>(&command))
            {
                renderer.beginClip(
                    antwika::gfx::RectF(
                        {static_cast<float>(
                             begun->rect.originPoint.x),
                         static_cast<float>(
                             begun->rect.originPoint.y)},
                        {static_cast<float>(
                             begun->rect.size.width),
                         static_cast<float>(
                             begun->rect.size.height)}));

                continue;
            }

            if (std::holds_alternative<PopClip>(command))
            {
                renderer.endClip();

                continue;
            }

            const auto &text = std::get<DrawText>(command);

            renderer.drawText(
                text.originPoint, text.text, text.scale, text.color);
        }
    }

}
