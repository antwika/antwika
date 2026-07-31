#pragma once

#include <gmock/gmock.h>

#include <antwika/sound/Frames.hpp>
#include <antwika/sound/IRenderCallback.hpp>
#include <antwika/sound/SampleBuffer.hpp>

namespace antwika::sound::mocks
{

    /**
     * @brief GMock double for IRenderCallback.
     */
    class MockRenderCallback : public IRenderCallback
    {
    public:
        MOCK_METHOD(
            void,
            render,
            (SampleBuffer out, FrameIndex firstFrame),
            (noexcept, override));
    };

} // namespace antwika::sound::mocks
