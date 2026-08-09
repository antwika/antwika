#pragma once

#include <span>

#include <antwika/gfx/Color.hpp>

namespace antwika::atlas_editor
{

    using antwika::gfx::Color;

    [[nodiscard]] std::span<const Color> defaultPalette() noexcept;

}
