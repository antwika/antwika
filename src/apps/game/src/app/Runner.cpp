#include "antwika/game/app/Runner.hpp"

#include <optional>
#include <utility>
#include <variant>

#include <antwika/image/PngFile.hpp>
#include <antwika/character/Character.hpp>
#include <antwika/component/AnimationState.hpp>
#include <antwika/component/Orientation.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/component/RosterIndex.hpp>
#include <antwika/gfx/Camera3D.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/MeshMaterial.hpp>
#include <antwika/gfx/SelectedBackend.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/WindowSpec.hpp>
#include <antwika/gfx/WindowEvent.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/SelectedInputBackend.hpp>
#include <antwika/light/ActiveLight.hpp>
#include <antwika/light/PointLight.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/map/MapAssets.hpp>
#include <antwika/gameplay/Roster.hpp>
#include <antwika/solver/VoxelWeave.hpp>
#include <antwika/tilemap/AtlasLayout.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/VoxelOcclusion.hpp>
#include <antwika/voxelmap/Voxel.hpp>
#include <antwika/collision/Collision.hpp>
#include <antwika/app/WindowEvents.hpp>

#include "antwika/game/app/Actions.hpp"

namespace antwika::game
{

    namespace
    {
        constexpr gfx::Color kBackgroundColor{
            .red = 6, .green = 6, .blue = 10, .alpha = 255};

        constexpr gfx::Color kSightPointColor{
            .red = 255, .green = 64, .blue = 64, .alpha = 255};

        constexpr float kSightPointSide = 3.0F;

        constexpr gfx::Color kOriginPointColor{
            .red = 255, .green = 255, .blue = 255, .alpha = 255};

        [[nodiscard]] gfx::Bitmap skinOf(
            const map::Map &laidMap,
            const std::string &mapPath,
            const std::size_t skinIndex)
        {
            const auto path =
                skinIndex < laidMap.characters.size()
                        && laidMap.characters.at(skinIndex).player
                    ? map::getSharedTexturePath(
                          mapPath, character::kCharacterSheet)
                    : map::getSidecarPath(
                          mapPath,
                          "figure-" + std::to_string(skinIndex)
                              + "-20x28.png");

            try
            {
                const auto loadedBitmap = image::getReadPngFile(path, kAppName);

                if (loadedBitmap.size == character::getCharacterSheetSize())
                {
                    return loadedBitmap;
                }
            }
            catch (...)
            {
            }

            return map::getLoadCharacterSheet(mapPath, kAppName);
        }
    }

    Runner::Runner(
        log::ILogger &logger,
        gfx::IGfxBackend &backendGiven,
        input::IInputBackend &inputsGiven,
        std::string mapPath)
        : mapPath(std::move(mapPath)),
          backend(backendGiven),
          inputs(inputsGiven),
          window(
              backend.createWindow(
                  gfx::WindowSpec{
                      .title = "Antwika",
                      .size = app::kDefaultWindowSize,
                      .resizable = true})),
          viewportRenderer(
              window->renderer(), window->getSize(), camera::kCanvasSize),
          map(map::getLoadMap(this->mapPath)),
          world(logger),
          playGame(logger, world, meshes.getCells(), patrolPositions)
    {
        patrolPositions = patrolStopsOf(map);
        meshes.rebuild(
            viewportRenderer,
            map,
            map.voxels,
            map.settings.cornersJoined
                ? solver::CornerSeams::Included
                : solver::CornerSeams::Ignored,
            map::getLoadAtlasPair(this->mapPath, kAppName),
            tick);
        sheets.open(
            viewportRenderer,
            map::getLoadAtlasPair(this->mapPath, kAppName),
            map,
            tick);

        std::vector<gfx::Bitmap> figureBitmaps;

        for (std::size_t index = 0; index < map.characters.size(); ++index)
        {
            figureBitmaps.push_back(skinOf(map, this->mapPath, index));
        }

        skins.take(viewportRenderer, std::move(figureBitmaps));
        worldShader.open(viewportRenderer, map::getLoadShader("voxel"));
        sprites.open(viewportRenderer);
        lightPasses.open(viewportRenderer, map::getLoadShader("shadow"));
        scenePass.open(viewportRenderer, map::getLoadShader("bloom"));

        playGame.setPlayer(
            gameplay::spawnRoster(
                world,
                map,
                map::getPlayerIndex(map).value_or(0),
                gameplay::getStartingPlacement(
                    map, meshes.getCells(), std::nullopt)));
        playGame.setWorldFrozen(false);
        playGame.setWalkerFrozen(false);
        playGame.getCameraTransform() =
            camera::getSnappedPitch(camera::getDefaultTransform());
        viewHeight =
            camera::getOrthoHalfHeight(camera::kCanvasSize, playGame.zoom());

        const auto stoodPosition =
            world.get<component::Position>(playGame.getPlayer());

        playGame.aimAt(
            getWorldRotation(), gfx::Vec3{stoodPosition.x, stoodPosition.y,
            stoodPosition.z});
        logger.log(log::Level::Info, "Playing " + this->mapPath);
    }

    void Runner::run()
    {
        tickDebt.start();

        while (window->isOpen() && running)
        {
            (void)tickDebt.advance();

            pollWindow();
            pollInputs();

            if (!window->isOpen() || !running)
            {
                break;
            }

            for (std::size_t tickCount = 0;
                 tickDebt.owesTick()
                 && tickCount < app::kMaxCatchUpTicks;
                 ++tickCount)
            {
                tickDebt.payTick();
                step();
            }

            if (tickDebt.owesTick())
            {
                tickDebt.forgive();
            }

            draw();
        }
    }

    void Runner::pollWindow()
    {
        const auto changes = app::windowChanges(backend, window->getId());

        if (changes.resizedSize.has_value())
        {
            viewportRenderer.resize(*changes.resizedSize);
        }

        if (changes.closeRequested)
        {
            running = false;
        }
    }

    void Runner::pollInputs()
    {
        inputState.beginTick();

        while (const auto event = inputs.pollEvent())
        {
            inputState.apply(*event);
        }

        if (actions.wasTriggered(kLeave, inputState))
        {
            running = false;
        }

        auto &walkKeys = playGame.wasdKeys();

        walkKeys.north = actions.isActive(kWalkNorth, inputState);
        walkKeys.south = actions.isActive(kWalkSouth, inputState);
        walkKeys.west = actions.isActive(kWalkWest, inputState);
        walkKeys.east = actions.isActive(kWalkEast, inputState);

        playGame.setRunning(actions.isActive(kRun, inputState));
    }

    void Runner::step()
    {
        playGame.stepAlongPath(true);
        playGame.run(tick);

        if (!world.isAlive(playGame.getPlayer()))
        {
            playGame.setPlayer(
                gameplay::spawnRoster(
                    world,
                    map,
                    map::getPlayerIndex(map).value_or(0),
                    gameplay::getStartingPlacement(
                        map,
                        meshes.getCells(),
                        playGame.getGates().checkpointPlacement)));
        }

        const auto stoodPosition =
            world.get<component::Position>(playGame.getPlayer());
        const gfx::Vec3 walkerPosition{stoodPosition.x, stoodPosition.y,
            stoodPosition.z};

        playGame.follow(getWorldRotation(), walkerPosition);
        ++tick;
    }

    gfx::Mat4 Runner::getWorldRotation() const
    {
        const auto orientation =
        world.get<component::Orientation>(playGame.getEye());

        return voxelmap::getModelRotation(orientation.yaw, orientation.pitch);
    }

    gfx::Camera3D Runner::getWorldCamera() const
    {
        return camera::cameraOf(
            playGame.getCameraTransform(),
            camera::kCanvasSize,
            viewHeight);
    }

    void Runner::drawSightPoints(
        const gfx::Mat4 &modelMatrix,
        const gfx::Camera3D &camera,
        const gfx::Vec3 walkerPosition,
        const bool upperSightOn)
    {
        const auto markAt =
            [this, &modelMatrix, &camera](
                const gfx::Vec3 position, const gfx::Color markColor)
        {
            const auto onCanvas = voxelmap::getProjectToScreen(
                camera, modelMatrix, camera::kCanvasSize, position);

            if (!onCanvas.has_value())
            {
                return;
            }

            viewportRenderer.drawRect(
                gfx::RectF{
                    gfx::PointF{
                        onCanvas->x - (kSightPointSide / 2.0F),
                        onCanvas->y - (kSightPointSide / 2.0F)},
                    gfx::SizeF{kSightPointSide, kSightPointSide}},
                markColor);
        };

        markAt(gfx::Vec3{0.0F, 0.0F, 0.0F}, kOriginPointColor);

        markAt(voxel::getLineOfSight(walkerPosition), kSightPointColor);

        if (upperSightOn)
        {
            markAt(
                voxel::getUpperLineOfSight(walkerPosition), kSightPointColor);
        }
    }

    void Runner::draw()
    {
        const auto modelMatrix = getWorldRotation();
        const auto camera = getWorldCamera();
        const auto stoodPosition =
            world.get<component::Position>(playGame.getPlayer());
        const gfx::Vec3 walkerPosition{stoodPosition.x, stoodPosition.y,
            stoodPosition.z};
        const auto sightPoint =
            voxel::getLineOfSight(walkerPosition);
        const auto upperSightPoint =
            voxel::getUpperLineOfSight(walkerPosition);
        const auto upperSightOn =
            !voxel::isCubeAbove(
                meshes.getCells(), walkerPosition, light::kSightClearance);
        auto lights = std::vector<light::ActiveLight>{
            light::ActiveLight{
                .position = sightPoint, .brightness = 0.0F}};

        if (upperSightOn)
        {
            lights.push_back(
                light::ActiveLight{
                    .position = upperSightPoint, .brightness = 0.0F});
        }

        for (const auto &light : light::getActiveLights(world, map.lamps))
        {
            if (lights.size() >= light::kMaxLamps)
            {
                break;
            }

            lights.push_back(light);
        }
        lightPasses.hide(
            viewportRenderer,
            voxel::getOccludingVoxels(meshes.getCells(), walkerPosition),
            voxel::VoxelPosition{
                .x = static_cast<std::int32_t>(
                    std::floor(walkerPosition.x / voxel::kVoxelSide)),
                .z = static_cast<std::int32_t>(
                    std::floor(walkerPosition.z / voxel::kVoxelSide))});
        lightPasses.bakeLamps(viewportRenderer, meshes.getSolid(), lights);
        sheets.refresh(
            viewportRenderer,
            map,
            tick,
            decor::isAnyTileAnimated(map.flipAnimations));

        if (decor::hasAnimatedDecor(map.decor)
            && tick % decor::kDecorPaceTick == 0)
        {
            meshes.rebuildDecor(viewportRenderer, map, tick);
        }

        worldShader.setLook(
            viewportRenderer,
            render::WorldShaderInputs{
                .playing = true,
                .lighting = map.settings.lighting,
                .sightOn = true,
                .ambient = static_cast<float>(map.ambient) / 100.0F,
                .walkerPosition = walkerPosition,
                .fadeAbove = stoodPosition.y,
                .carrying =
                    light::getCarriedLightSlot(world, playGame.getPlayer()),
                .hidingCornerPosition = voxelmap::getOcclusionMaskOrigin(
                    voxel::VoxelPosition{
                        .x = static_cast<std::int32_t>(
                            std::floor(
                                walkerPosition.x / voxel::kVoxelSide)),
                        .z = static_cast<std::int32_t>(
                            std::floor(
                                walkerPosition.z / voxel::kVoxelSide))}),
                .sightPoint = sightPoint,
                .sightSlot = 0,
                .upperSightPoint = upperSightPoint,
                .upperSightSlot = 1,
                .upperSightOn = upperSightOn},
            lights,
            lightPasses.getLamps());

        const auto material = [this](const bool keyed)
        {
            return gfx::MeshMaterial{
                .texture =
                    keyed ? sheets.getKeyed(tilemap::Atlas::Floor)
                          : sheets.getTexture(tilemap::Atlas::Floor),
                .materialMapTexture =
                    keyed ? sheets.getKeyed(tilemap::Atlas::Wall)
                          : sheets.getTexture(tilemap::Atlas::Wall),
                .shadowMapTexture = lightPasses.getHiding(),
                .pointLightShadowAtlasTexture = lightPasses.getLampShadows(),
                .shader = &worldShader.getProgram()};
        };

        const auto pile = [&]
        {
            for (const auto &piece : meshes.getSolid())
            {
                viewportRenderer.drawMesh(
                    *piece,
                    modelMatrix,
                    camera,
                    material(false));
            }

            if (meshes.getDecor() != nullptr)
            {
                viewportRenderer.drawMesh(
                    *meshes.getDecor(), modelMatrix, camera, material(true));
            }

            for (const auto &piece : meshes.getWater())
            {
                viewportRenderer.drawMesh(*piece,
                    modelMatrix,
                    camera,
                    material(false));
            }
        };

        scenePass.draw(
            viewportRenderer,
            worldShader.getProgram(),
            kBackgroundColor,
            pile,
            [&]
            {
                const auto ground = collision::getGroundHeightUnderFootprint(
                    meshes.getCells(), stoodPosition.x, stoodPosition.z,
                        stoodPosition.y);

                if (ground.has_value())
                {
                    sprites.drawShadow(
                        viewportRenderer,
                        camera,
                        modelMatrix,
                        gfx::Vec3{
                            stoodPosition.x, *ground + 0.02F, stoodPosition.z});
                }

                for (const auto entity :
                     world.view<
                         component::Position,
                         component::AnimationState,
                         component::RosterIndex>())
                {
                    sprites.drawCharacter(
                        viewportRenderer,
                        worldShader.getProgram(),
                        camera,
                        modelMatrix,
                        skins.getPicture(
                            world.get<component::RosterIndex>(entity).index),
                        world.get<component::Position>(entity),
                        world.get<component::AnimationState>(entity),
                        tick,
                        lightPasses.getLampShadows());
                }
            });

        drawSightPoints(
            modelMatrix, camera, walkerPosition, upperSightOn);

        viewportRenderer.fillLetterbox(gfx::Color{});
        viewportRenderer.present();
    }

}
