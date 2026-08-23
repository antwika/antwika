#include "antwika/ui/Painter.hpp"

#include <variant>

#include <antwika/gfx/RectF.hpp>

#include "antwika/ui/DrawCommand.hpp"

namespace antwika::ui
{

    namespace
    {
        template <typename... Arms>
        struct Overloaded final : Arms...
        {
            using Arms::operator()...;
        };
    }


    void paint(ISurfaceRenderer &renderer, const DrawList &drawList)
    {
        for (const auto &command : drawList)
        {
            std::visit(
                Overloaded{
                    [&renderer](const FillRect &fill)
                    {
                        renderer.drawRect(fill.rect, fill.color);
                    },
                    [&renderer](const DrawTexture &picture)
                    {
                        if (picture.texture == nullptr)
                        {
                            return;
                        }

                        renderer.drawTexture(
                            *picture.texture,
                            picture.sourceRect,
                            picture.destinationRect,
                            picture.tintColor);
                    },
                    [&renderer](const PushClip &clip)
                    {
                        renderer.beginClip(
                            antwika::gfx::RectF(
                                {static_cast<float>(
                                     clip.rect.originPoint.x),
                                 static_cast<float>(
                                     clip.rect.originPoint.y)},
                                {static_cast<float>(
                                     clip.rect.size.width),
                                 static_cast<float>(
                                     clip.rect.size.height)}));
                    },
                    [&renderer](const PopClip &) { renderer.endClip(); },
                    [&renderer](const DrawText &text)
                    {
                        renderer.drawText(
                            text.originPoint,
                            text.text,
                            text.scale,
                            text.color);
                    }},
                command);
        }
    }

}
