#pragma once

#include <gmock/gmock.h>

#include <cstddef>

#include <antwika/gfx/IMesh.hpp>

namespace antwika::gfx::mocks
{

    using antwika::gfx::IMesh;

    /**
     * @brief GMock double for IMesh.
     */
    class MockMesh : public IMesh
    {
    public:
        MOCK_METHOD(std::size_t, vertexCount, (), (const, override));
        MOCK_METHOD(std::size_t, triangleCount, (), (const, override));
    };

} // namespace antwika::gfx::mocks
