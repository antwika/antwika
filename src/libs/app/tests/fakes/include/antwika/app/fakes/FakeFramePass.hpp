#pragma once

#include <cstddef>
#include <vector>

#include <antwika/animation/Progress.hpp>
#include <antwika/app/IFramePass.hpp>

namespace antwika::app::fakes
{

    using antwika::animation::Progress;
    using antwika::app::IFramePass;

    class FakeFramePass final : public IFramePass
    {
    public:
        void draw(Progress subTickProgress) override
        {
            frames.push_back(subTickProgress);
        }

        [[nodiscard]] const std::vector<Progress> &getDrawnProgress() const noexcept
        {
            return frames;
        }

        [[nodiscard]] std::size_t getCount() const noexcept
        {
            return frames.size();
        }

    private:
        std::vector<Progress> frames;
    };

}
