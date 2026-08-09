#pragma once

#include <gmock/gmock.h>

#include <memory>

#include <antwika/gfx/Camera3D.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/IMesh.hpp>
#include <antwika/gfx/IRenderer3D.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/MeshData.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>

namespace antwika::gfx::mocks
{

    using antwika::gfx::IRenderer3D;

    class MockRenderer3D
        : public MockRenderer
        , public IRenderer3D
    {
    public:
        MOCK_METHOD(
            std::unique_ptr<IMesh>,
            createMesh,
            (const MeshData &mesh),
            (override));

        MOCK_METHOD(
            void,
            drawMesh,
            (const IMesh &mesh,
             const Mat4 &model,
             const Camera3D &camera,
             Color tint),
            (override));

        [[nodiscard]] IRenderer3D *renderer3d() override
        {
            return this;
        }
    };

}
