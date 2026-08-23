#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>

#include <antwika/log/Level.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/gfx/Camera3D.hpp"
#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/GfxError.hpp"
#include "antwika/gfx/IMesh.hpp"
#include "antwika/gfx/IRenderer.hpp"
#include "antwika/gfx/IShader.hpp"
#include "antwika/gfx/Math3D.hpp"
#include "antwika/gfx/MeshData.hpp"
#include "antwika/gfx/MeshMaterial.hpp"
#include "antwika/gfx/ShaderSource.hpp"

#include "NullRenderer.hpp"

using antwika::gfx::Camera3D;
using antwika::gfx::Color;
using antwika::gfx::GfxError;
using antwika::gfx::getIdentityMatrix;
using antwika::gfx::IMesh;
using antwika::gfx::IShader;
using antwika::gfx::MeshData;
using antwika::gfx::MeshMaterial;
using antwika::gfx::ShaderSource;
using antwika::gfx::Vec3;
using antwika::gfx::Vertex3D;
using antwika::gfx::detail::NullRenderer;
using antwika::log::Level;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

namespace
{
    MeshData getOneTriangle()
    {
        return MeshData{
            .vertices =
                {Vertex3D{.position = Vec3{0.0F, 0.0F, 0.0F}},
                 Vertex3D{.position = Vec3{1.0F, 0.0F, 0.0F}},
                 Vertex3D{.position = Vec3{0.0F, 1.0F, 0.0F}}},
            .indices = {0U, 1U, 2U}};
    }
}

TEST(NullRenderer3DTest, CreateMesh_ReportsTheCountsItWasGiven)
{
    NiceMock<MockLogger> logger;
    NullRenderer renderer(logger);

    EXPECT_CALL(logger, log(Level::Trace, "gfx.null: create mesh"));

    const std::unique_ptr<IMesh> mesh =
        renderer.createMesh(getOneTriangle());

    ASSERT_NE(nullptr, mesh);
    EXPECT_EQ(3U, mesh->getVertexCount());
    EXPECT_EQ(1U, mesh->getTriangleCount());
}

TEST(NullRenderer3DTest, CreateMesh_RefusesGeometryARealBackendWould)
{
    NiceMock<MockLogger> logger;
    NullRenderer renderer(logger);

    MeshData mesh = getOneTriangle();
    mesh.indices.back() = 3U;

    EXPECT_THROW(
        { (void)renderer.createMesh(mesh); }, GfxError);
}

TEST(NullRenderer3DTest, DrawMesh_DiscardsTheDrawAndTracesIt)
{
    NiceMock<MockLogger> logger;
    NullRenderer renderer(logger);

    const std::unique_ptr<IMesh> mesh =
        renderer.createMesh(getOneTriangle());

    EXPECT_CALL(logger, log(Level::Trace, "gfx.null: draw mesh"));

    renderer.drawMesh(
        *mesh, getIdentityMatrix(), Camera3D{}, Color{});
}

TEST(NullRenderer3DTest, CreateShader_ReturnsAReadyProgramAndTracesIt)
{
    NiceMock<MockLogger> logger;
    NullRenderer renderer(logger);

    EXPECT_CALL(logger, log(Level::Trace, "gfx.null: create shader"));

    const std::unique_ptr<IShader> shader = renderer.createShader(
        ShaderSource{.vertex = "vs", .fragment = "fs"});

    ASSERT_NE(nullptr, shader);
    EXPECT_TRUE(shader->isReady());
}

TEST(NullRenderer3DTest, CreateShader_RefusesSourceARealBackendWould)
{
    NiceMock<MockLogger> logger;
    NullRenderer renderer(logger);

    EXPECT_THROW(
        { (void)renderer.createShader(ShaderSource{}); }, GfxError);
}

TEST(NullRenderer3DTest, DrawMesh_DiscardsAMaterialDrawAndTracesIt)
{
    NiceMock<MockLogger> logger;
    NullRenderer renderer(logger);

    const std::unique_ptr<IMesh> mesh =
        renderer.createMesh(getOneTriangle());
    const std::unique_ptr<IShader> shader = renderer.createShader(
        ShaderSource{.vertex = "vs", .fragment = "fs"});

    EXPECT_CALL(logger, log(Level::Trace, "gfx.null: draw mesh"));

    renderer.drawMesh(
        *mesh,
        getIdentityMatrix(),
        Camera3D{},
        MeshMaterial{.shader = shader.get()});
}
