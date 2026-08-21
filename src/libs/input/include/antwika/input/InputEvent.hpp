#pragma once

#include <cstdint>
#include <variant>

#include "antwika/input/Key.hpp"
#include "antwika/input/KeyModifiers.hpp"
#include "antwika/input/MouseButton.hpp"
#include "antwika/input/Position.hpp"
#include "antwika/input/event/KeyPressed.hpp"
#include "antwika/input/event/KeyReleased.hpp"
#include "antwika/input/event/PointerButtonPressed.hpp"
#include "antwika/input/event/PointerButtonReleased.hpp"
#include "antwika/input/event/PointerMoved.hpp"
#include "antwika/input/event/PointerScrolled.hpp"

namespace antwika::input
{

    using InputEvent = std::variant<
        KeyPressed,
        KeyReleased,
        PointerMoved,
        PointerButtonPressed,
        PointerButtonReleased,
        PointerScrolled>;

}
