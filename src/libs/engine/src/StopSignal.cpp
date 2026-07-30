#include "antwika/engine/StopSignal.hpp"

#include "antwika/engine/Events.hpp"

namespace antwika::engine
{

    void StopSignal::handle(const TickEvent &event)
    {
        if (event.event.name == events::kStop)
        {
            isStopped = true;
        }
    }

    bool StopSignal::stopped() const noexcept
    {
        return isStopped;
    }

} // namespace antwika::engine
