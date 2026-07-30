#pragma once

#include <gmock/gmock.h>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Rect.hpp>

namespace antwika::gfx::mocks
{

    using antwika::gfx::IRenderer;

    /**
     * @brief GMock double for IRenderer.
     */
    class MockRenderer : public IRenderer
    {
    public:
        MOCK_METHOD(void, clear, (Color color), (override));
        MOCK_METHOD(void, drawRect, (Rect rect, Color color), (override));
        MOCK_METHOD(void, present, (), (override));
    };

} // namespace antwika::gfx::mocks
