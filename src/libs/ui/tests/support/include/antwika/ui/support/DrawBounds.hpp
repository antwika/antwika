#pragma once

#include <gtest/gtest.h>

#include <cstdint>
#include <variant>

#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/TextLayout.hpp>

#include "antwika/ui/DrawCommand.hpp"
#include "antwika/ui/DrawList.hpp"

namespace antwika::ui::support
{

    inline void expectInsideCanvas(
        const DrawList &commands, const gfx::Size canvas)
    {
        const auto right = static_cast<std::int32_t>(canvas.width);
        const auto bottom = static_cast<std::int32_t>(canvas.height);

        for (const auto &command : commands)
        {
            if (const auto *fill = std::get_if<FillRect>(&command))
            {
                EXPECT_GE(fill->rect.origin.x, 0);
                EXPECT_GE(fill->rect.origin.y, 0);
                EXPECT_LE(
                    fill->rect.origin.x
                        + static_cast<std::int32_t>(fill->rect.size.width),
                    right);
                EXPECT_LE(
                    fill->rect.origin.y
                        + static_cast<std::int32_t>(fill->rect.size.height),
                    bottom);

                continue;
            }

            const auto &text = std::get<DrawText>(command);
            const auto extent = gfx::textSize(text.text, text.scale);

            EXPECT_GE(text.origin.x, 0);
            EXPECT_GE(text.origin.y, 0);
            EXPECT_LE(
                text.origin.x + static_cast<std::int32_t>(extent.width),
                right);
            EXPECT_LE(
                text.origin.y + static_cast<std::int32_t>(extent.height),
                bottom);
        }
    }

}
