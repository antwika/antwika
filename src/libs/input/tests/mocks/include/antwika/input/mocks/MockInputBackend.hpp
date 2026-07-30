#pragma once

#include <gmock/gmock.h>

#include <optional>
#include <string_view>

#include <antwika/input/IInputBackend.hpp>
#include <antwika/input/InputCapabilities.hpp>
#include <antwika/input/InputEvent.hpp>

namespace antwika::input::mocks
{

    using antwika::input::IInputBackend;

    /**
     * @brief GMock double for IInputBackend.
     */
    class MockInputBackend : public IInputBackend
    {
    public:
        MOCK_METHOD(std::string_view, name, (), (const, override));

        MOCK_METHOD(
            InputCapabilities, capabilities, (), (const, override));

        MOCK_METHOD(std::optional<InputEvent>, pollEvent, (), (override));
    };

} // namespace antwika::input::mocks
