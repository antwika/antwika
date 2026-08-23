#pragma once

#include <gmock/gmock.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <string_view>

#include <antwika/gfx/GfxCapabilities.hpp>
#include <antwika/gfx/IGfxBackend.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/WindowSpec.hpp>
#include <antwika/gfx/WindowEvent.hpp>

namespace antwika::gfx::mocks
{

    using antwika::gfx::IGfxBackend;

    class MockGfxBackend : public IGfxBackend
    {
    public:
        MOCK_METHOD(std::string_view, name, (), (const, override));
        MOCK_METHOD(std::size_t, maxWindows, (), (const, override));
        MOCK_METHOD(
            GfxCapabilities, capabilities, (), (const, override));

        MOCK_METHOD(
            std::unique_ptr<IWindow>,
            createWindow,
            (const WindowSpec &spec),
            (override));

        MOCK_METHOD(std::optional<WindowEvent>, pollEvent, (), (override));
    };

}
