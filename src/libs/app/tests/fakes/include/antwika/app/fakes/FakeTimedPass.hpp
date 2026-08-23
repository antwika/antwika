#pragma once

#include <chrono>
#include <vector>

#include <antwika/animation/Progress.hpp>
#include <antwika/app/IFramePass.hpp>
#include <antwika/time/fakes/FakeClock.hpp>

namespace antwika::app::fakes
{

    using antwika::animation::Progress;
    using antwika::time::fakes::FakeClock;

    class FakeTimedPass final : public IFramePass
    {
    public:
        FakeTimedPass(FakeClock &clock, std::chrono::milliseconds each)
            : clock(clock), each(each)
        {
        }

        void draw(Progress subTickProgress) override
        {
            progresses.push_back(subTickProgress);
            clock.advance(each);
        }

        [[nodiscard]] const std::vector<Progress> &getDrawnProgress() const noexcept
        {
            return progresses;
        }

    private:
        FakeClock &clock;
        std::chrono::milliseconds each;
        std::vector<Progress> progresses;
    };

}
