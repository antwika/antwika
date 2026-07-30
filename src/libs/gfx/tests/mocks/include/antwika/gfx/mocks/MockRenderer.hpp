#pragma once

#include <gmock/gmock.h>

#include <cstdint>
#include <string_view>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Point.hpp>
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
        MOCK_METHOD(
            void,
            drawText,
            (Point origin,
             std::string_view text,
             std::uint32_t scale,
             Color color),
            (override));
        MOCK_METHOD(void, present, (), (override));
    };

} // namespace antwika::gfx::mocks
