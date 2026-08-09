#pragma once

#include <optional>

#include <antwika/input/Key.hpp>

namespace antwika::sudoku
{

    [[nodiscard]] std::optional<int> digitFor(
        antwika::input::Key key) noexcept;

}
