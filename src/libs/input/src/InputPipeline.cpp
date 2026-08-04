#include "antwika/input/InputPipeline.hpp"
#include <antwika/event/ITickEventSource.hpp>

namespace antwika::input
{

    using antwika::event::ITickEventSource;

    InputPipeline::InputPipeline(
        ITickEventSource &inner,
        IInputBackend &backend,
        const IInputEventCodec &codec,
        const InputPipelineOptions &options)
        : outermost(&inner)
    {
        if (options.readsDevice)
        {
            live.emplace(*outermost, backend, codec);
            outermost = &*live;

            // Nested rather than beside it, which is the whole rule.
            // What a device says is in the device's own coordinates.
            // What a file holds has already been through this.
            if (options.pointerMapping)
            {
                mapping.emplace(
                    *outermost, codec, options.pointerMapping->get());
                outermost = &*mapping;
            }
        }

        if (options.pointerHint)
        {
            hinting.emplace(*outermost, codec, options.pointerHint->get());
            outermost = &*hinting;
        }

        if (options.coalescePointerMotion)
        {
            coalescing.emplace(*outermost);
            outermost = &*coalescing;
        }

        if (options.thinIdleMotion)
        {
            idle.emplace(*outermost, codec);
            outermost = &*idle;
        }

        if (options.stopOnKey)
        {
            stopping.emplace(*outermost, codec, *options.stopOnKey);
            outermost = &*stopping;
        }
    }

    std::vector<Event> InputPipeline::eventsFor(antwika::time::Tick tick)
    {
        return outermost->eventsFor(tick);
    }

} // namespace antwika::input
