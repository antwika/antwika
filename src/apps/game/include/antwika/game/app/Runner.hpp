#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/camera/FlyCamera.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/gfx/IGfxBackend.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/input/IInputBackend.hpp>
#include <antwika/input/InputState.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/map/MapFile.hpp>
#include <antwika/render/AtlasSheets.hpp>
#include <antwika/render/CharacterSkins.hpp>
#include <antwika/render/LightPasses.hpp>
#include <antwika/render/ScenePass.hpp>
#include <antwika/render/Sprites.hpp>
#include <antwika/render/WorldMeshes.hpp>
#include <antwika/render/WorldShader.hpp>
#include <antwika/time/SystemClock.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/voxel/VoxelCell.hpp>

#include <antwika/gameplay/Game.hpp>

#include "antwika/game/app/Actions.hpp"

namespace antwika::game
{

    inline constexpr std::string_view kAppName = "antwika_gameplay";

    inline constexpr std::chrono::milliseconds kTickPeriod{16};

    inline constexpr std::size_t kMaxCatchUpTicks = 5;

    class Runner final
    {
    public:
        Runner(log::ILogger &logger, std::string mapPath);

        void run();

    private:
        void pollWindow();

        void pollInputs();

        void step();

        void draw();

        void drawSightPoints(
            const gfx::Mat4 &modelMatrix,
            const gfx::Camera3D &camera,
            gfx::Vec3 walkerPosition);

        [[nodiscard]] gfx::Mat4 worldRotation() const;

        [[nodiscard]] gfx::Camera3D worldCamera() const;

        std::string mapPath;
        std::unique_ptr<gfx::IGfxBackend> backend;
        std::unique_ptr<input::IInputBackend> inputs;
        std::unique_ptr<gfx::IWindow> window;
        gfx::ViewportRenderer viewportRenderer;
        map::Map map;
        ecs::World world;
        std::vector<std::vector<voxel::VoxelCell>> patrolCells;
        render::WorldMeshes meshes;
        render::AtlasSheets sheets;
        render::CharacterSkins skins;
        render::LightPasses lightPasses;
        render::ScenePass scenePass;
        render::Sprites sprites;
        render::WorldShader worldShader;
        gameplay::Game playGame;
        input::InputState inputState;
        input::ActionMap actions = defaultActions();

        time::Tick tick = 0;
        std::chrono::nanoseconds tickDebt{};
        time::SystemClock clockSource;
        std::chrono::time_point<std::chrono::system_clock> lastFrameAt;
        float viewHeight = 1.0F;
        bool running = true;
    };

}
