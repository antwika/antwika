#include "antwika/engine/StopSignal.hpp"

#include "antwika/engine/Events.hpp"

namespace antwika::engine
{

    void StopSignal::handle(const TickEvent &event)
    {
        if (event.event.name == events::kStop)
        {
            stopped_ = true;
        }
    }

    bool StopSignal::stopped() const noexcept
    {
        return stopped_;
    }

} // namespace antwika::engine
