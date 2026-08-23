#pragma once

#include <gmock/gmock.h>

#include <string>
#include <string_view>

#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/WindowId.hpp>

namespace antwika::gfx::mocks
{

    using antwika::gfx::IWindow;

    class MockWindow : public IWindow
    {
    public:
        MOCK_METHOD(WindowId, id, (), (const, override));
        MOCK_METHOD(bool, isOpen, (), (const, override));
        MOCK_METHOD(std::string, title, (), (const, override));
        MOCK_METHOD(Size, configuredSize, (), (const, override));
        MOCK_METHOD(Size, size, (), (const, override));
        MOCK_METHOD(bool, isFullscreen, (), (const, override));
        MOCK_METHOD(IRenderer &, renderer, (), (override));
        MOCK_METHOD(void, setTitle, (std::string_view title), (override));
        MOCK_METHOD(void, setSize, (Size size), (override));
        MOCK_METHOD(void, setFullscreen, (bool fullscreen), (override));
        MOCK_METHOD(void, close, (), (override));
    };

}
