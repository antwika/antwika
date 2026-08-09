#pragma once

#include <gmock/gmock.h>

#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/Size.hpp>

namespace antwika::gfx::mocks
{

    using antwika::gfx::ITexture;

    class MockTexture : public ITexture
    {
    public:
        MOCK_METHOD(Size, size, (), (const, override));
    };

}
