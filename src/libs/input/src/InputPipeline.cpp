#include "antwika/input/InputPipeline.hpp"

namespace antwika::input
{

    InputPipeline::InputPipeline(
        IReplaySource &inner,
        IInputBackend &backend,
        const IInputEventCodec &codec,
        const InputPipelineOptions &options)
        : outermost(&inner)
    {
        if (options.readsDevice)
        {
            live.emplace(*outermost, backend, codec);
            outermost = &*live;
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
