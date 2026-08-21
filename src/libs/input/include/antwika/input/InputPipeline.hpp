#pragma once

#include <functional>
#include <optional>
#include <vector>

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
#include "antwika/input/InputPipelineOptions.hpp"
#include "antwika/input/Key.hpp"
#include "antwika/input/LiveInputSource.hpp"
#include "antwika/input/MappedPointerSource.hpp"
#include "antwika/input/PointerHintChannel.hpp"
#include "antwika/input/PointerHintSource.hpp"
#include "antwika/input/StopOnKeySource.hpp"

namespace antwika::input
{

    using antwika::event::Event;
    using antwika::event::ITickEventSource;

    class InputPipeline final : public ITickEventSource
    {
    public:
        InputPipeline(
            ITickEventSource &innerSource,
            IInputBackend &backend,
            const IInputEventCodec &codec,
            const InputPipelineOptions &options);

        InputPipeline(const InputPipeline &) = delete;
        InputPipeline(InputPipeline &&) = delete;

        InputPipeline &operator=(const InputPipeline &) = delete;
        InputPipeline &operator=(InputPipeline &&) = delete;

        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

        [[nodiscard]] std::optional<std::reference_wrapper<IFramePump>>
        framePump() noexcept;

    private:
        std::optional<LiveInputSource> liveSource;
        std::optional<MappedPointerSource> mappingSource;
        std::optional<PointerHintSource> hinting;
        std::optional<BufferedInputSource> bufferingSource;
        std::optional<CoalescingPointerSource> coalescing;
        std::optional<IdleMotionFilter> idle;
        std::optional<StopOnKeySource> stopping;

        ITickEventSource *chainHeadSource;
    };

}
