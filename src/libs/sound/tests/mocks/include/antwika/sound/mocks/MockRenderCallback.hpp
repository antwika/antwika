#pragma once

#include <gmock/gmock.h>

#include <antwika/sound/Frames.hpp>
#include <antwika/sound/IRenderCallback.hpp>
#include <antwika/sound/SampleBuffer.hpp>

namespace antwika::sound::mocks
{

    class MockRenderCallback : public IRenderCallback
    {
    public:
        MOCK_METHOD(
            void,
            render,
            (SampleBuffer outputBuffer, FrameIndex firstFrame),
            (noexcept, override));
    };

}
