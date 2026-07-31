#include "antwika/input/PointerHintChannel.hpp"

namespace antwika::input
{

    void PointerHintChannel::publish(PointerHint hint) noexcept
    {
        latest = hint;
    }

    std::optional<PointerHint>
    PointerHintChannel::forRenderingOnly() const noexcept
    {
        return latest;
    }

} // namespace antwika::input
