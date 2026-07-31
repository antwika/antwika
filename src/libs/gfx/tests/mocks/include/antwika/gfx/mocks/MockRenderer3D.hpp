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

    /**
     * @brief GMock double for a renderer that has a 3D half.
     *
     * Both halves in one object, as NullRenderer and RaylibRenderer are,
     * because that is the shape an application meets: it asks a renderer
     * for its 3D half and is handed the same frame back.
     * renderer3d() is concrete rather than mocked, since a double that
     * could be told to disown its own 3D methods would only let a test
     * assert something no backend does.
     */
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

        /**
         * @brief Offer this double's 3D half.
         * @return This object, which is also an IRenderer3D.
         */
        [[nodiscard]] IRenderer3D *renderer3d() override
        {
            return this;
        }
    };

} // namespace antwika::gfx::mocks
