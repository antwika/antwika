#pragma once

#include <cstddef>
#include <functional>

#include "antwika/editor/view/IEditSteps.hpp"

namespace antwika::editor::fakes
{

    class FakeEditSteps final : public IEditSteps
    {
    public:
        void pushUndo() override
        {
            ++pushCount;

            if (pushProbe)
            {
                pushProbe();
            }
        }

        std::size_t pushCount = 0;

        std::function<void()> pushProbe;
    };

}
