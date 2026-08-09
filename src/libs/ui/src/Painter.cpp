#include "antwika/ui/Painter.hpp"

#include <variant>

#include "antwika/ui/DrawCommand.hpp"

namespace antwika::ui
{

    void paint(IRenderer &renderer, const DrawList &commands)
    {
        for (const auto &command : commands)
        {
            if (const auto *fill = std::get_if<FillRect>(&command))
            {
                renderer.drawRect(fill->rect, fill->color);

                continue;
            }

            const auto &text = std::get<DrawText>(command);

            renderer.drawText(
                text.origin, text.text, text.scale, text.color);
        }
    }

}
