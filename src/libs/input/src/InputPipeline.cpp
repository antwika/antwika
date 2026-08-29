#include "antwika/input/InputPipeline.hpp"

#include <antwika/event/ITickEventSource.hpp>

namespace antwika::input
{

    using antwika::event::ITickEventSource;

    InputPipeline::InputPipeline(
        ITickEventSource &innerSource,
        IInputBackend &backend,
        const IInputEventCodec &codec,
        const InputPipelineOptions &options)
        : chainHeadSource(&innerSource)
    {
        if (options.readsDevice)
        {
            liveSource.emplace(*chainHeadSource, backend, codec);
            chainHeadSource = &*liveSource;

            if (options.pointerMapping)
            {
                mappingSource.emplace(
                    *chainHeadSource, codec, options.pointerMapping->get());
                chainHeadSource = &*mappingSource;
            }
        }

        if (options.pointerHint)
        {
            hinting.emplace(
                *chainHeadSource,
                codec,
                options.pointerHint->get());
            chainHeadSource = &*hinting;
        }

        if (options.readsDevice)
        {
            bufferingSource.emplace(*chainHeadSource);
            chainHeadSource = &*bufferingSource;
        }

        if (options.coalescePointerMotion)
        {
            coalescing.emplace(*chainHeadSource);
            chainHeadSource = &*coalescing;
        }

        if (options.suppressIdleMotion)
        {
            idle.emplace(*chainHeadSource, codec);
            chainHeadSource = &*idle;
        }

        if (options.stopOnKey)
        {
            stopping.emplace(*chainHeadSource, codec, *options.stopOnKey);
            chainHeadSource = &*stopping;
        }
    }

    std::vector<Event> InputPipeline::eventsFor(antwika::time::Tick tick)
    {
        return chainHeadSource->eventsFor(tick);
    }

    std::optional<std::reference_wrapper<IFramePump>>
    InputPipeline::framePump() noexcept
    {
        if (!bufferingSource.has_value())
        {
            return std::nullopt;
        }

        return *bufferingSource;
    }

}
