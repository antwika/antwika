#pragma once

#include <cstdint>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/atlas_editor/Pixel.hpp"

namespace antwika::atlas_editor
{

    using antwika::gfx::Bitmap;
    using antwika::gfx::Color;
    using antwika::gfx::Size;

    class Canvas final
    {
    public:
        explicit Canvas(Bitmap image, std::uint64_t revision = 0);

        [[nodiscard]] static Canvas blank(Size size);

        [[nodiscard]] Size size() const noexcept;

        [[nodiscard]] Color at(Pixel pixel) const noexcept;

        [[nodiscard]] bool holds(Pixel pixel) const noexcept;

        bool set(Pixel pixel, Color color) noexcept;

        [[nodiscard]] const Bitmap &bitmap() const noexcept;

        [[nodiscard]] std::uint64_t revision() const noexcept;

    private:
        Bitmap image;
        std::uint64_t changes = 0;
    };

}
