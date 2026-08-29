#include <optional>
#include <vector>

#include <antwika/event/TickEvent.hpp>
#include <antwika/event/conformance/TickEventSourceConformanceTest.hpp>

#include "antwika/replay/ReplaySource.hpp"

namespace antwika::event::conformance
{

    namespace
    {

        struct ReplaySourceTraits final
        {
            [[nodiscard]] ITickEventSource &getSource()
            {
                if (!source)
                {
                    source.emplace(std::vector<TickEvent>{});
                }

                return *source;
            }

            void raiseTwoEvents()
            {
                source.emplace(std::vector<TickEvent>{
                    TickEvent{
                        .tick = kRaiseTick,
                        .event = getRaisedEvents().front()},
                    TickEvent{
                        .tick = kRaiseTick,
                        .event = getRaisedEvents().back()}});
            }

            std::optional<replay::ReplaySource> source;
        };

    }

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Replay, TickEventSourceConformanceTest, ReplaySourceTraits);

}
