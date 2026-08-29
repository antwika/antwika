#pragma once

#include <utility>

#include <antwika/event/ITickEventSource.hpp>
#include <antwika/event/conformance/TickEventSourceConformanceTest.hpp>
#include <antwika/event/fakes/FakeScriptedTickSource.hpp>

namespace antwika::event::conformance
{

    template <typename Source>
    struct ScriptedSourceTraits
    {
        template <typename... Args>
        explicit ScriptedSourceTraits(Args &&...args)
            : source(innerSource, std::forward<Args>(args)...)
        {
        }

        [[nodiscard]] ITickEventSource &getSource()
        {
            return source;
        }

        void raiseTwoEvents()
        {
            innerSource.nextEvents = getRaisedEvents();
        }

        fakes::FakeScriptedTickSource innerSource;
        Source source;
    };

}
