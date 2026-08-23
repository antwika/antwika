#pragma once

#include <gmock/gmock.h>

#include <cstddef>

#include <antwika/gfx/IMesh.hpp>

namespace antwika::gfx::mocks
{

    using antwika::gfx::IMesh;

    class MockMesh : public IMesh
    {
    public:
        MOCK_METHOD(std::size_t, getVertexCount, (), (const, override));
        MOCK_METHOD(std::size_t, getTriangleCount, (), (const, override));
    };

}
