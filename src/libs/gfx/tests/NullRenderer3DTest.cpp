#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>

#include <antwika/log/Level.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/gfx/Camera3D.hpp"
#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/GfxError.hpp"
#include "antwika/gfx/IMesh.hpp"
#include "antwika/gfx/IRenderer3D.hpp"
#include "antwika/gfx/Math3D.hpp"
#include "antwika/gfx/MeshData.hpp"

#include "NullRenderer.hpp"

using antwika::gfx::Camera3D;
using antwika::gfx::Color;
using antwika::gfx::GfxError;
using antwika::gfx::identityMatrix;
using antwika::gfx::IMesh;
using antwika::gfx::IRenderer3D;
using antwika::gfx::MeshData;
using antwika::gfx::Vec3;
using antwika::gfx::Vertex3D;
using antwika::gfx::detail::NullRenderer;
using antwika::log::Level;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

namespace
{
    MeshData oneTriangle()
    {
        return MeshData{
            .vertices =
                {Vertex3D{.position = Vec3{0.0F, 0.0F, 0.0F}},
                 Vertex3D{.position = Vec3{1.0F, 0.0F, 0.0F}},
                 Vertex3D{.position = Vec3{0.0F, 1.0F, 0.0F}}},
            .indices = {0U, 1U, 2U}};
    }
} // namespace

TEST(NullRenderer3DTest, Renderer3d_OffersItselfAsTheThreeDeeHalf)
{
    NiceMock<MockLogger> logger;
    NullRenderer renderer(logger);

    IRenderer3D *renderer3d = renderer.renderer3d();

    ASSERT_NE(nullptr, renderer3d);
    EXPECT_EQ(
        static_cast<IRenderer3D *>(&renderer), renderer3d);
}

TEST(NullRenderer3DTest, CreateMesh_ReportsTheCountsItWasGiven)
{
    NiceMock<MockLogger> logger;
    NullRenderer renderer(logger);

    EXPECT_CALL(logger, log(Level::Trace, "gfx.null: create mesh"));

    const std::unique_ptr<IMesh> mesh =
        renderer.createMesh(oneTriangle());

    ASSERT_NE(nullptr, mesh);
    EXPECT_EQ(3U, mesh->vertexCount());
    EXPECT_EQ(1U, mesh->triangleCount());
}

TEST(NullRenderer3DTest, CreateMesh_RefusesGeometryARealBackendWould)
{
    NiceMock<MockLogger> logger;
    NullRenderer renderer(logger);

    MeshData mesh = oneTriangle();
    mesh.indices.back() = 3U;

    EXPECT_THROW(
        { (void)renderer.createMesh(mesh); }, GfxError);
}

TEST(NullRenderer3DTest, DrawMesh_DiscardsTheDrawAndTracesIt)
{
    NiceMock<MockLogger> logger;
    NullRenderer renderer(logger);

    const std::unique_ptr<IMesh> mesh =
        renderer.createMesh(oneTriangle());

    EXPECT_CALL(logger, log(Level::Trace, "gfx.null: draw mesh"));

    renderer.drawMesh(
        *mesh, identityMatrix(), Camera3D{}, Color{});
}
