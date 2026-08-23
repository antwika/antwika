#pragma once

#include <gmock/gmock.h>

#include <optional>

#include <antwika/event/Event.hpp>
#include <antwika/input/IInputEventCodec.hpp>
#include <antwika/input/InputEvent.hpp>

namespace antwika::input::mocks
{

    using antwika::event::Event;
    using antwika::input::IInputEventCodec;

    class MockInputEventCodec : public IInputEventCodec
    {
    public:
        MOCK_METHOD(
            Event, getEncode, (const InputEvent &event), (const, override));

        MOCK_METHOD(
            std::optional<InputEvent>,
            getDecode,
            (const Event &event),
            (const, override));
    };

}
