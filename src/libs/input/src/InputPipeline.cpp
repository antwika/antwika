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

        if (options.readsDevice)
        {
            buffering.emplace(*outermost);
            outermost = &*buffering;
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

    std::optional<std::reference_wrapper<IFramePump>>
    InputPipeline::framePump() noexcept
    {
        if (!buffering.has_value())
        {
            return std::nullopt;
        }

        return *buffering;
    }

}
