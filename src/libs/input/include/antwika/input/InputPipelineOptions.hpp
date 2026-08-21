#pragma once

#include <optional>
#include <antwika/event/Event.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>
#include "antwika/input/BufferedInputSource.hpp"
#include "antwika/input/CoalescingPointerSource.hpp"
#include "antwika/input/IFramePump.hpp"
#include "antwika/input/IInputBackend.hpp"
#include "antwika/input/IInputEventCodec.hpp"
#include "antwika/input/IPointerMapping.hpp"
#include "antwika/input/IdleMotionFilter.hpp"
#include "antwika/input/Key.hpp"
#include "antwika/input/LiveInputSource.hpp"
#include "antwika/input/MappedPointerSource.hpp"
#include "antwika/input/PointerHintChannel.hpp"
#include "antwika/input/PointerHintSource.hpp"
#include "antwika/input/StopOnKeySource.hpp"

namespace antwika::input
{

    struct InputPipelineOptions final
    {
        bool readsDevice = true;

        std::optional<std::reference_wrapper<const IPointerMapping>>
            pointerMapping = std::nullopt;

        bool coalescePointerMotion = false;

        bool suppressIdleMotion = false;

        std::optional<std::reference_wrapper<PointerHintChannel>>
            pointerHint = std::nullopt;

        std::optional<Key> stopOnKey = std::nullopt;
    };

}
