#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/game/Messages.hpp"
#include "antwika/game/SceneSnapshot.hpp"

namespace antwika::game
{

    using antwika::gfx::Color;
    using antwika::gfx::Point;
    using antwika::gfx::Rect;
    using antwika::gfx::Size;

    struct ReadoutLine final
    {
        std::string text;

        Point origin;

        Color colour;

        [[nodiscard]] bool operator==(const ReadoutLine &other) const
            = default;
    };

    struct ReadoutPanel final
    {
        Rect box;

        std::vector<ReadoutLine> lines;

        [[nodiscard]] bool operator==(const ReadoutPanel &other) const
            = default;
    };

    inline constexpr Color kReadoutBackdrop{
        .red = 16, .green = 18, .blue = 24, .alpha = 225};

    inline constexpr Color kReadoutTitle{
        .red = 236, .green = 238, .blue = 236};

    inline constexpr std::uint32_t kReadoutTextScale = 2;

    [[nodiscard]] ReadoutPanel readoutPanel(
        const HoverReadout &readout,
        Size canvas,
        const Translator &translator);

}
