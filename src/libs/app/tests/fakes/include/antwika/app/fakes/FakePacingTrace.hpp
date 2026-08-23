#pragma once

#include <cstdint>
#include <vector>

#include <antwika/animation/Progress.hpp>
#include <antwika/app/IFramePass.hpp>
#include <antwika/input/IFramePump.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::app::fakes
{

    using antwika::animation::Progress;
    using antwika::app::IFramePass;
    using antwika::input::IFramePump;

    enum class PacingStep : std::uint8_t
    {
        Pumped = 0,
        Drawn = 1,
    };

    class FakePacingTrace final : public IFramePass, public IFramePump
    {
    public:
        void pollFrame(antwika::time::Tick tick) override
        {
            steps.push_back(PacingStep::Pumped);
            sampledTicks.push_back(tick);
        }

        void draw(Progress) override
        {
            steps.push_back(PacingStep::Drawn);
        }

        [[nodiscard]] const std::vector<PacingStep> &recordedSteps()
            const noexcept
        {
            return steps;
        }

        [[nodiscard]] const std::vector<antwika::time::Tick> &ticks()
            const noexcept
        {
            return sampledTicks;
        }

    private:
        std::vector<PacingStep> steps;
        std::vector<antwika::time::Tick> sampledTicks;
    };

}
