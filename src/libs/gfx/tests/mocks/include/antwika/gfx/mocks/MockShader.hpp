#pragma once

#include <gmock/gmock.h>

#include <antwika/gfx/IShader.hpp>

namespace antwika::gfx::mocks
{

    using antwika::gfx::IShader;

    class MockShader : public IShader
    {
    public:
        MOCK_METHOD(bool, isReady, (), (const, override));
    };

}
