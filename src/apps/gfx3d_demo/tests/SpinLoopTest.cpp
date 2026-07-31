#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <optional>

#include <antwika/gfx/GfxError.hpp>
#include <antwika/gfx/IMesh.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/MeshData.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/gfx/WindowEvent.hpp>
#include <antwika/gfx/mocks/MockGfxBackend.hpp>
#include <antwika/gfx/mocks/MockMesh.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockRenderer3D.hpp>
#include <antwika/gfx/mocks/MockWindow.hpp>

#include "antwika/gfx3d_demo/CubeMesh.hpp"
#include "antwika/gfx3d_demo/SpinLoop.hpp"
#include "antwika/gfx3d_demo/SpinScene.hpp"

using antwika::gfx::CloseRequested;
using antwika::gfx::GfxError;
using antwika::gfx::IMesh;
using antwika::gfx::IWindow;
using antwika::gfx::Resized;
using antwika::gfx::Size;
using antwika::gfx::WindowDesc;
using antwika::gfx::WindowEvent;
using antwika::gfx::WindowId;
using antwika::gfx::mocks::MockGfxBackend;
using antwika::gfx::mocks::MockMesh;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockRenderer3D;
using antwika::gfx::mocks::MockWindow;
using antwika::gfx3d_demo::cubeMesh;
using antwika::gfx3d_demo::SpinLoop;
using antwika::gfx3d_demo::SpinScene;
using ::testing::_;
using ::testing::ByMove;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace
{
    constexpr Size kCanvas{.width = 800, .height = 600};

    /**
     * @brief One backend, one window and one renderer, wired together
     * the way a run finds them.
     *
     * The renderer is a MockRenderer3D, which is both halves in one
     * object exactly as NullRenderer and RaylibRenderer are.
     */
    struct SpinFixture
    {
        NiceMock<MockGfxBackend> backend;
        NiceMock<MockRenderer3D> renderer;
        MockWindow *window = nullptr;

        static constexpr WindowId kOurWindow{1};
        static constexpr WindowId kSomeoneElsesWindow{99};

        void expectOneWindow(bool open)
        {
            wireWindow(open, renderer);

            // WillOnce is the assertion here.
            // The upload happens once a run, not once a frame.
            EXPECT_CALL(renderer, createMesh(_))
                .WillOnce(
                    Return(ByMove(std::unique_ptr<IMesh>(
                        std::make_unique<NiceMock<MockMesh>>()))));
        }

        void wireWindow(bool open, antwika::gfx::IRenderer &into)
        {
            auto owned = std::make_unique<NiceMock<MockWindow>>();
            window = owned.get();

            ON_CALL(*window, id()).WillByDefault(Return(kOurWindow));
            ON_CALL(*window, isOpen()).WillByDefault(Return(open));
            ON_CALL(*window, renderer()).WillByDefault(ReturnRef(into));
            ON_CALL(*window, size()).WillByDefault(Return(kCanvas));

            EXPECT_CALL(backend, createWindow(_))
                .WillOnce(Return(ByMove(
                    std::unique_ptr<IWindow>(std::move(owned)))));
        }
    };
} // namespace

TEST(SpinLoopTest, Run_UploadsTheCubeOnceAndDrawsEachFrame)
{
    SpinFixture fixture;
    fixture.expectOneWindow(true);

    ON_CALL(fixture.backend, pollEvent())
        .WillByDefault(Return(std::nullopt));

    EXPECT_CALL(fixture.renderer, drawMesh(_, _, _, _)).Times(3);
    EXPECT_CALL(fixture.renderer, present()).Times(3);

    const SpinScene scene;
    SpinLoop loop(fixture.backend, scene);

    loop.run(WindowDesc{.title = "Antwika"}, cubeMesh(), 3);

    EXPECT_EQ(loop.ticks(), 3u);
}

TEST(SpinLoopTest, Run_TurnsTheCubeFurtherEveryFrame)
{
    SpinFixture fixture;
    fixture.expectOneWindow(true);

    ON_CALL(fixture.backend, pollEvent())
        .WillByDefault(Return(std::nullopt));

    const SpinScene scene;

    // The frames must differ, and differ by their tick.
    // Not by however long the frame before them took.
    const ::testing::InSequence order;

    EXPECT_CALL(fixture.renderer, drawMesh(_, scene.modelAt(0), _, _));
    EXPECT_CALL(fixture.renderer, drawMesh(_, scene.modelAt(1), _, _));

    SpinLoop loop(fixture.backend, scene);

    loop.run(WindowDesc{.title = "Antwika"}, cubeMesh(), 2);
}

TEST(SpinLoopTest, Run_ClosesTheWindowWhenTheBackendReportsACloseRequest)
{
    SpinFixture fixture;
    fixture.expectOneWindow(false);

    EXPECT_CALL(fixture.backend, pollEvent())
        .WillOnce(
            Return(WindowEvent{
                .window = SpinFixture::kOurWindow,
                .payload = CloseRequested{}}))
        .WillRepeatedly(Return(std::nullopt));

    EXPECT_CALL(fixture.renderer, present()).Times(0);

    const SpinScene scene;
    SpinLoop loop(fixture.backend, scene);

    // Once for the close request and once on the way out.
    // Closing an already-closed window is part of IWindow's contract.
    EXPECT_CALL(*fixture.window, close()).Times(2);

    loop.run(WindowDesc{.title = "Antwika"}, cubeMesh(), 10);

    EXPECT_EQ(loop.ticks(), 0u);
}

TEST(SpinLoopTest, Run_IgnoresAnEventForSomebodyElsesWindow)
{
    SpinFixture fixture;
    fixture.expectOneWindow(true);

    EXPECT_CALL(fixture.backend, pollEvent())
        .WillOnce(
            Return(WindowEvent{
                .window = SpinFixture::kSomeoneElsesWindow,
                .payload = CloseRequested{}}))
        .WillRepeatedly(Return(std::nullopt));

    EXPECT_CALL(fixture.renderer, present()).Times(1);

    const SpinScene scene;
    SpinLoop loop(fixture.backend, scene);

    loop.run(WindowDesc{.title = "Antwika"}, cubeMesh(), 1);
}

TEST(SpinLoopTest, Run_KeepsDrawingThroughEventsThatAreNotCloseRequests)
{
    SpinFixture fixture;
    fixture.expectOneWindow(true);

    EXPECT_CALL(fixture.backend, pollEvent())
        .WillOnce(
            Return(WindowEvent{
                .window = SpinFixture::kOurWindow,
                .payload = Resized{.size = kCanvas}}))
        .WillRepeatedly(Return(std::nullopt));

    EXPECT_CALL(*fixture.window, close()).Times(1);
    EXPECT_CALL(fixture.renderer, present()).Times(1);

    const SpinScene scene;
    SpinLoop loop(fixture.backend, scene);

    loop.run(WindowDesc{.title = "Antwika"}, cubeMesh(), 1);
}

TEST(SpinLoopTest, Run_StopsAtOnceWhenTheWindowIsAlreadyClosed)
{
    SpinFixture fixture;
    fixture.expectOneWindow(false);

    ON_CALL(fixture.backend, pollEvent())
        .WillByDefault(Return(std::nullopt));

    // An uncapped run is the one the null backend gets.
    // A closed window is the only thing that ends it.
    EXPECT_CALL(fixture.renderer, present()).Times(0);

    const SpinScene scene;
    SpinLoop loop(fixture.backend, scene);

    loop.run(WindowDesc{.title = "Antwika"}, cubeMesh(), std::nullopt);

    EXPECT_EQ(loop.ticks(), 0u);
}

TEST(SpinLoopTest, Run_DrawsNothingWhenNoFramesAreAskedFor)
{
    SpinFixture fixture;
    fixture.expectOneWindow(true);

    EXPECT_CALL(fixture.renderer, present()).Times(0);
    EXPECT_CALL(*fixture.window, close()).Times(1);

    const SpinScene scene;
    SpinLoop loop(fixture.backend, scene);

    loop.run(WindowDesc{.title = "Antwika"}, cubeMesh(), 0);
}

TEST(SpinLoopTest, Run_RefusesABackendWithNo3DRenderer)
{
    SpinFixture fixture;
    NiceMock<MockRenderer> flatOnly;

    // IRenderer::renderer3d() returns null by default.
    // That is how a backend says it has no 3D path.
    // This whole application is one mesh.
    // Saying so beats drawing an empty window.
    fixture.wireWindow(true, flatOnly);

    const SpinScene scene;
    SpinLoop loop(fixture.backend, scene);

    EXPECT_THROW(
        loop.run(WindowDesc{.title = "Antwika"}, cubeMesh(), 1),
        GfxError);
}
