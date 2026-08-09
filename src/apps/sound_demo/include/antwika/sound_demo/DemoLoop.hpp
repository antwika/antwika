#pragma once

#include <vector>

#include <antwika/sound/DeviceDesc.hpp>
#include <antwika/sound/Frames.hpp>
#include <antwika/sound/ISoundBackend.hpp>
#include <antwika/sound/PlayRequest.hpp>
#include <antwika/sound/WaveformLibrary.hpp>
#include <antwika/time/ISleeper.hpp>

namespace antwika::sound_demo
{

    using antwika::sound::DeviceDesc;
    using antwika::sound::FrameCount;
    using antwika::sound::ISoundBackend;
    using antwika::sound::PlayRequest;
    using antwika::sound::WaveformLibrary;
    using antwika::time::ISleeper;

    class DemoLoop final
    {
    public:
        DemoLoop(
            ISoundBackend &backend,
            const WaveformLibrary &library,
            ISleeper &sleeper);

        DemoLoop(const DemoLoop &) = delete;
        DemoLoop(DemoLoop &&) = delete;

        DemoLoop &operator=(const DemoLoop &) = delete;
        DemoLoop &operator=(DemoLoop &&) = delete;

        void run(
            const DeviceDesc &desc,
            const std::vector<PlayRequest> &notes,
            FrameCount frames);

        [[nodiscard]] FrameCount rendered() const noexcept;

    private:
        ISoundBackend &backend;
        const WaveformLibrary &library;
        ISleeper &sleeper;

        FrameCount renderedFrames = 0;
    };

}
