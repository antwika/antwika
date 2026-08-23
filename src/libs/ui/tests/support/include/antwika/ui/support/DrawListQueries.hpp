#pragma once

#include <string>
#include <variant>
#include <vector>

#include <antwika/gfx/Color.hpp>

#include "antwika/ui/DrawCommand.hpp"
#include "antwika/ui/DrawList.hpp"

namespace antwika::ui::support
{

    [[nodiscard]] inline std::vector<std::string> textsOf(
        const DrawList &drawList)
    {
        std::vector<std::string> texts;

        for (const auto &command : drawList)
        {
            if (const auto *text = std::get_if<DrawText>(&command))
            {
                texts.push_back(text->text);
            }
        }

        return texts;
    }

    [[nodiscard]] inline std::vector<FillRect> getFillsColored(
        const DrawList &drawList, const gfx::Color fillColor)
    {
        std::vector<FillRect> fillRects;

        for (const auto &command : drawList)
        {
            const auto *fill = std::get_if<FillRect>(&command);

            if (fill != nullptr && fill->color == fillColor)
            {
                fillRects.push_back(*fill);
            }
        }

        return fillRects;
    }

}
