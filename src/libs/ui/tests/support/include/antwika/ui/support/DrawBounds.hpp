#pragma once

#include <gtest/gtest.h>

#include <cstdint>
#include <variant>

#include <antwika/gfx/Size.hpp>
#include <antwika/text/TextLayout.hpp>

#include "antwika/ui/DrawCommand.hpp"
#include "antwika/ui/DrawList.hpp"

namespace antwika::ui::support
{

    inline void expectInsideCanvas(
        const DrawList &drawList, const gfx::Size canvasSize)
    {
        const auto right = static_cast<std::int32_t>(canvasSize.width);
        const auto bottom = static_cast<std::int32_t>(canvasSize.height);

        for (const auto &command : drawList)
        {
            if (const auto *fill = std::get_if<FillRect>(&command))
            {
                EXPECT_GE(fill->rect.originPoint.x, 0);
                EXPECT_GE(fill->rect.originPoint.y, 0);
                EXPECT_LE(
                    fill->rect.originPoint.x
                        + static_cast<std::int32_t>(fill->rect.size.width),
                    right);
                EXPECT_LE(
                    fill->rect.originPoint.y
                        + static_cast<std::int32_t>(fill->rect.size.height),
                    bottom);

                continue;
            }

            const auto &text = std::get<DrawText>(command);
            const auto extent = text::textSize(text.text, text.scale);

            EXPECT_GE(text.originPoint.x, 0);
            EXPECT_GE(text.originPoint.y, 0);
            EXPECT_LE(
                text.originPoint.x + static_cast<std::int32_t>(extent.width),
                right);
            EXPECT_LE(
                text.originPoint.y + static_cast<std::int32_t>(extent.height),
                bottom);
        }
    }

}
