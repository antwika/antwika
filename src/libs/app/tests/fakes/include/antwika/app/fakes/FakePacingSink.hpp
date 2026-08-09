#pragma once

#include <cstdint>
#include <vector>

#include <antwika/app/IFramePacingSink.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::app::fakes
{

    enum class PacedFrame : std::uint8_t
    {
        Drawn = 0,
        Dropped = 1,
    };

    class FakePacingSink final : public IFramePacingSink
    {
    public:
        void drew(antwika::time::Tick tick) override
        {
            frames.push_back(PacedFrame::Drawn);
            ticks.push_back(tick);
        }

        void dropped(antwika::time::Tick tick) override
        {
            frames.push_back(PacedFrame::Dropped);
            ticks.push_back(tick);
        }

        [[nodiscard]] const std::vector<PacedFrame> &told() const noexcept
        {
            return frames;
        }

        [[nodiscard]] const std::vector<antwika::time::Tick> &about()
            const noexcept
        {
            return ticks;
        }

    private:
        std::vector<PacedFrame> frames;
        std::vector<antwika::time::Tick> ticks;
    };

}
