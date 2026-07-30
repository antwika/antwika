#pragma once

#include <gmock/gmock.h>

#include <cstdint>
#include <memory>
#include <string_view>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/ITexture.hpp>
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
        MOCK_METHOD(
            std::unique_ptr<ITexture>,
            createTexture,
            (const Bitmap &bitmap),
            (override));
        MOCK_METHOD(
            void,
            drawTexture,
            (const ITexture &texture,
             Rect source,
             Rect destination,
             Color tint),
            (override));
        MOCK_METHOD(void, present, (), (override));
    };

} // namespace antwika::gfx::mocks
