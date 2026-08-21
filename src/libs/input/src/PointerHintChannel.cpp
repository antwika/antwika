#include "antwika/input/PointerHintChannel.hpp"

namespace antwika::input
{

    void PointerHintChannel::publish(PointerHint hint) noexcept
    {
        latestHint = hint;
    }

    std::optional<PointerHint>
    PointerHintChannel::latest() const noexcept
    {
        return latestHint;
    }

}
