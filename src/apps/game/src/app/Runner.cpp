#include "antwika/game/app/Runner.hpp"

#include <optional>
#include <utility>
#include <variant>

#include <antwika/image/PngFile.hpp>
#include <antwika/character/Character.hpp>
#include <antwika/component/AnimationState.hpp>
#include <antwika/component/Orientation.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/component/CharacterIndex.hpp>
#include <antwika/gfx/Camera3D.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/GfxError.hpp>
#include <antwika/gfx/MeshBox.hpp>
#include <antwika/gfx/MeshMaterial.hpp>
#include <antwika/gfx/SelectedBackend.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/WindowSpec.hpp>
#include <antwika/gfx/WindowEvent.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/SelectedInputBackend.hpp>
#include <antwika/component/DirectionKeys.hpp>
#include <antwika/light/ActiveLight.hpp>
#include <antwika/light/PointLight.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/assets/MapAssets.hpp>
#include <antwika/assets/ShaderAssets.hpp>
#include <antwika/gameplay/Characters.hpp>
#include <antwika/gameplay/PadReports.hpp>
#include <antwika/solver/VoxelWeave.hpp>
#include <antwika/tilemap/AtlasLayout.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/VoxelOcclusion.hpp>
#include <antwika/voxelmap/Voxel.hpp>
#include <antwika/collision/Collision.hpp>
#include <antwika/app/FramePacing.hpp>
#include <antwika/app/WindowEvents.hpp>

#include "antwika/game/app/Actions.hpp"

namespace antwika::game
{

    namespace
    {
        constexpr gfx::Color kBackgroundColor{
            .red = 6, .green = 6, .blue = 10, .alpha = 255};

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
                          "character-" + std::to_string(skinIndex)
                              + "-20x28.png");

            try
            {
                const auto loadedBitmap = image::getReadPngFile(path, kAppName);

                if (loadedBitmap.size == character::getCharacterSheetSize())
                {
                    return loadedBitmap;
                }
            }
            catch (const gfx::GfxError &)
            {
            }

            return assets::getLoadCharacterSheet(mapPath, kAppName);
        }
    }

    Runner::Runner(
        log::ILogger &logger,
        gfx::IGfxBackend &backendGiven,
        input::IInputBackend &inputsGiven,
        std::string mapPath)
        : mapPath(std::move(mapPath)),
          logger(logger),
          backend(backendGiven),
          inputs(inputsGiven),
          window(
              backend.createWindow(
                  gfx::WindowSpec{
                      .title = "Antwika game",
                      .size = app::kDefaultWindowSize,
                      .resizable = true,
                      .targetFps = app::kTargetFps})),
          viewportRenderer(
              window->renderer(), window->getSize(), camera::kCanvasSize),
          map(map::getLoadMap(this->mapPath)),
          world(logger),
          patrolPositions(patrolStopsOf(map)),
          playGame(logger, world, map, meshes.getCells(), patrolPositions)
    {
        meshes.rebuild(
            viewportRenderer,
            map,
            map.voxels,
            map.settings.cornersJoined
                ? solver::CornerSeams::Included
                : solver::CornerSeams::Ignored,
            assets::getLoadAtlasPair(this->mapPath, kAppName),
            tick);
        sheets.open(
            viewportRenderer,
            assets::getLoadAtlasPair(this->mapPath, kAppName),
            map,
            tick);

        std::vector<gfx::Bitmap> characterBitmaps;

        for (std::size_t index = 0; index < map.characters.size(); ++index)
        {
            characterBitmaps.push_back(skinOf(map, this->mapPath, index));
        }

        skins.take(viewportRenderer, std::move(characterBitmaps));
        worldShader.open(viewportRenderer, assets::getShaderSource("voxel"));
        sprites.open(viewportRenderer);
        lightPasses.open(viewportRenderer, assets::getShaderSource("shadow"));
        scenePass.open(viewportRenderer, assets::getShaderSource("bloom"));

        gameplay::spawnCharacters(
            world, map, map::getPlayerIndex(map).value_or(0));
        playGame.standPlayer();
        playGame.setSimulation(simulationState);
        playGame.getCameraTransform() =
            camera::getSnappedPitch(camera::getDefaultTransform());
        viewHeight =
            camera::getOrthoHalfHeight(camera::kCanvasSize, playGame.getZoom());

        const auto stoodPosition =
            world.get<component::Position>(playGame.getPlayer());

        playGame.aimAt(
            getWorldRotation(), collision::positionOf(stoodPosition));
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
                 running && tickDebt.owesTick()
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

        playGame.setWasdKeys(
            component::DirectionKeys{
                .north = actions.isActive(kWalkNorth, inputState),
                .south = actions.isActive(kWalkSouth, inputState),
                .west = actions.isActive(kWalkWest, inputState),
                .east = actions.isActive(kWalkEast, inputState)});

        simulationState.running = actions.isActive(kRun, inputState);
        playGame.setSimulation(simulationState);
    }

    void Runner::step()
    {
        playGame.stepAlongPath(true);
        playGame.run(tick);

        static_cast<void>(
            gameplay::takeCheckpointReport(
                playGame, world, playGame.getPlayer()));

        if (gameplay::takeExitReport(world, playGame.getPlayer()))
        {
            logger.log(log::Level::Info, "the exit was reached");

            running = false;
        }

        const auto stoodPosition =
            world.get<component::Position>(playGame.getPlayer());

        playGame.follow(
            getWorldRotation(), collision::positionOf(stoodPosition));
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

    Runner::Frame Runner::getFrame() const
    {
        const auto stoodPosition =
            world.get<component::Position>(playGame.getPlayer());
        const auto walkerPosition = collision::positionOf(stoodPosition);

        return Frame{
            .modelMatrix = getWorldRotation(),
            .camera = getWorldCamera(),
            .stoodPosition = stoodPosition,
            .walkerPosition = walkerPosition,
            .sightPoint = voxel::getLineOfSight(walkerPosition),
            .upperSightPoint = voxel::getUpperLineOfSight(walkerPosition),
            .walkerVoxelPosition =
                voxel::VoxelPosition{
                    .x = collision::columnOf(walkerPosition.x),
                    .z = collision::columnOf(walkerPosition.z)},
            .upperSightOn =
                !voxel::isCubeAbove(
                    meshes.getCells(),
                    walkerPosition,
                    light::kSightClearance)};
    }

    std::vector<light::ActiveLight> Runner::getFrameLights(
        const Frame &frame) const
    {
        auto lights = std::vector<light::ActiveLight>{
            light::ActiveLight{
                .position = frame.sightPoint, .brightness = 0.0F}};

        if (frame.upperSightOn)
        {
            lights.push_back(
                light::ActiveLight{
                    .position = frame.upperSightPoint, .brightness = 0.0F});
        }

        for (const auto &light : light::getActiveLights(world, map.lamps))
        {
            if (lights.size() >= light::kMaxLamps)
            {
                break;
            }

            lights.push_back(light);
        }

        return lights;
    }

    void Runner::bakeLightPasses(
        const Frame &frame, const std::vector<light::ActiveLight> &lights)
    {
        lightPasses.hide(
            viewportRenderer,
            voxel::getOccludingVoxels(
                meshes.getCells(), frame.walkerPosition),
            frame.walkerVoxelPosition);
        lightPasses.bakeLamps(viewportRenderer, meshes.getSolid(), lights);
    }

    void Runner::refreshDecor()
    {
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
    }

    void Runner::setWorldLook(
        const Frame &frame, const std::vector<light::ActiveLight> &lights)
    {
        worldShader.setLook(
            viewportRenderer,
            render::WorldShaderInputs{
                .playing = true,
                .lighting = map.settings.lighting,
                .sightOn = true,
                .ambient = static_cast<float>(map.ambient) / 100.0F,
                .walkerPosition = frame.walkerPosition,
                .fadeAbove = frame.stoodPosition.y,
                .carrying =
                    light::getCarriedLightSlot(world, playGame.getPlayer()),
                .hidingCornerPosition =
                    voxelmap::getOcclusionMaskOrigin(frame.walkerVoxelPosition),
                .sightPoint = frame.sightPoint,
                .sightSlot = 0,
                .upperSightPoint = frame.upperSightPoint,
                .upperSightSlot = 1,
                .upperSightOn = frame.upperSightOn,
                .viewPosition = frame.camera.getPosition(),
                .viewTargetPoint = frame.camera.getTarget(),
                .backdropColor = kBackgroundColor},
            lights,
            lightPasses.getLamps());
    }

    void Runner::drawPile(const Frame &frame)
    {
        const auto material = [this](const bool keyed)
        {
            return gfx::MeshMaterial{
                .texture =
                    keyed ? sheets.getKeyedTexture(tilemap::Atlas::Floor)
                          : sheets.getTexture(tilemap::Atlas::Floor),
                .materialMapTexture =
                    keyed ? sheets.getKeyedTexture(tilemap::Atlas::Wall)
                          : sheets.getTexture(tilemap::Atlas::Wall),
                .shadowMapTexture = lightPasses.getHiding(),
                .pointLightShadowAtlasTexture = lightPasses.getLampShadows(),
                .shader = &worldShader.getProgram()};
        };

        const auto clipMatrix =
            frame.camera.getViewProjection() * frame.modelMatrix;

        for (const auto &piece : meshes.getSolid())
        {
            if (gfx::isBoxOutside(piece.box, clipMatrix))
            {
                continue;
            }

            viewportRenderer.drawMesh(
                *piece.mesh,
                frame.modelMatrix,
                frame.camera,
                material(false));
        }

        if (meshes.getDecor() != nullptr)
        {
            viewportRenderer.drawMesh(
                *meshes.getDecor(),
                frame.modelMatrix,
                frame.camera,
                material(true));
        }

        for (const auto &piece : meshes.getWater())
        {
            if (gfx::isBoxOutside(piece.box, clipMatrix))
            {
                continue;
            }

            viewportRenderer.drawMesh(
                *piece.mesh,
                frame.modelMatrix,
                frame.camera,
                material(false));
        }
    }

    void Runner::drawCharacters(const Frame &frame)
    {
        const auto ground = collision::getGroundHeightUnderFootprint(
            meshes.getCells(),
            frame.stoodPosition.x,
            frame.stoodPosition.z,
            frame.stoodPosition.y);

        if (ground.has_value())
        {
            sprites.drawShadow(
                viewportRenderer,
                frame.camera,
                frame.modelMatrix,
                gfx::Vec3{
                    frame.stoodPosition.x,
                    *ground + 0.02F,
                    frame.stoodPosition.z});
        }

        for (const auto entity :
             world.view<
                 component::Position,
                 component::AnimationState,
                 component::CharacterIndex>())
        {
            sprites.drawCharacter(
                viewportRenderer,
                worldShader.getProgram(),
                frame.camera,
                frame.modelMatrix,
                skins.getPicture(
                    world.get<component::CharacterIndex>(entity).index),
                world.get<component::Position>(entity),
                world.get<component::AnimationState>(entity),
                tick,
                lightPasses.getLampShadows());
        }
    }

    void Runner::drawScene(const Frame &frame)
    {
        scenePass.draw(
            viewportRenderer,
            worldShader.getProgram(),
            kBackgroundColor,
            [this, &frame]
            {
                drawPile(frame);
            },
            [this, &frame]
            {
                drawCharacters(frame);
            });
    }

    void Runner::draw()
    {
        const auto frame = getFrame();
        const auto lights = getFrameLights(frame);

        bakeLightPasses(frame, lights);
        refreshDecor();
        setWorldLook(frame, lights);
        drawScene(frame);
        viewportRenderer.fillLetterbox(gfx::Color{});
        viewportRenderer.present();
    }

}
