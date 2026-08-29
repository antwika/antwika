#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/app/FramePacing.hpp>
#include <antwika/app/TickDebt.hpp>
#include <antwika/camera/FlyCamera.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/gfx/Camera3D.hpp>
#include <antwika/gfx/IGfxBackend.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/input/IInputBackend.hpp>
#include <antwika/input/InputState.hpp>
#include <antwika/light/ActiveLight.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/map/MapFile.hpp>
#include <antwika/render/AtlasSheets.hpp>
#include <antwika/render/CharacterSkins.hpp>
#include <antwika/render/LightPasses.hpp>
#include <antwika/render/ScenePass.hpp>
#include <antwika/render/Sprites.hpp>
#include <antwika/render/WorldMeshes.hpp>
#include <antwika/render/WorldShader.hpp>
#include <antwika/system/SimulationState.hpp>
#include <antwika/time/SystemClock.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/voxel/VoxelPosition.hpp>

#include <antwika/gameplay/Game.hpp>

#include "antwika/game/app/Actions.hpp"

namespace antwika::game
{

    inline constexpr std::string_view kAppName = "antwika_game";


    class Runner final
    {
    public:
        Runner(
            log::ILogger &logger,
            gfx::IGfxBackend &backend,
            input::IInputBackend &inputs,
            std::string mapPath);

        void run();

    private:
        struct Frame final
        {
            gfx::Mat4 modelMatrix;
            gfx::Camera3D camera;
            component::Position stoodPosition;
            gfx::Vec3 walkerPosition;
            gfx::Vec3 sightPoint;
            gfx::Vec3 upperSightPoint;
            voxel::VoxelPosition walkerVoxelPosition;
            bool upperSightOn = false;
        };

        void pollWindow();

        void pollInputs();

        void step();

        void draw();

        [[nodiscard]] Frame getFrame() const;

        [[nodiscard]] std::vector<light::ActiveLight> getFrameLights(
            const Frame &frame) const;

        void bakeLightPasses(
            const Frame &frame,
            const std::vector<light::ActiveLight> &lights);

        void refreshDecor();

        void setWorldLook(
            const Frame &frame,
            const std::vector<light::ActiveLight> &lights);

        void drawScene(const Frame &frame);

        void drawPile(const Frame &frame);

        void drawCharacters(const Frame &frame);

        [[nodiscard]] gfx::Mat4 getWorldRotation() const;

        [[nodiscard]] gfx::Camera3D getWorldCamera() const;

        std::string mapPath;
        log::ILogger &logger;
        gfx::IGfxBackend &backend;
        input::IInputBackend &inputs;
        std::unique_ptr<gfx::IWindow> window;
        gfx::ViewportRenderer viewportRenderer;
        map::Map map;
        ecs::World world;
        std::vector<std::vector<voxel::VoxelPosition>> patrolPositions;
        render::WorldMeshes meshes;
        render::AtlasSheets sheets;
        render::CharacterSkins skins;
        render::LightPasses lightPasses;
        render::ScenePass scenePass;
        render::Sprites sprites;
        render::WorldShader worldShader;
        gameplay::Game playGame;
        system::SimulationState simulationState;
        input::InputState inputState;
        input::ActionMap actions = getDefaultActions();

        time::Tick tick = 0;
        time::SystemClock clockSource;
        app::TickDebt tickDebt{clockSource};
        float viewHeight = 1.0F;
        bool running = true;
    };

}
