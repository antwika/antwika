#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <antwika/engine/Events.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include "antwika/input/InputEvent.hpp"
#include "antwika/input/InputEventCodec.hpp"
#include "antwika/input/InputState.hpp"
#include "antwika/input/MouseButton.hpp"
#include "antwika/input/Position.hpp"

namespace antwika::input::fakes
{

    struct SessionSummary final
    {
        Position pointerPosition;
        std::vector<std::string> pressedKeys;
        std::uint32_t clicks = 0;
        std::int32_t scrollTotal = 0;
        bool leftHeldAtEnd = false;

        [[nodiscard]] bool operator==(
            const SessionSummary &other) const = default;
    };

}
