#include <filesystem>

#include <antwika/io/AssetPath.hpp>
#include <antwika/gfx/PngFile.hpp>
#include <antwika/app/SpawnDetached.hpp>
#include <antwika/component/AnimationState.hpp>
#include <antwika/component/Player.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/component/RosterIndex.hpp>
#include <antwika/component/Speaker.hpp>
#include <antwika/editor/ui/EditorLook.hpp>
#include <antwika/gfx/Camera3D.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/MeshMaterial.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/map/MapAssets.hpp>
#include <antwika/gameplay/Roster.hpp>
#include <antwika/voxel/VoxelCell.hpp>
#include <antwika/voxel/VoxelOcclusion.hpp>

#include "antwika/editor/Editor.hpp"

namespace
{

    constexpr float kTalkRadius = 1.6F;

}

namespace antwika::editor
{

    std::string Editor::characterSheetPath(
        const std::size_t figureIndex) const
    {
        if (figureIndex < map.characters.size()
            && map.characters.at(figureIndex).player)
        {
            return map::sharedTexturePath(mapPath, character::kCharacterSheet);
        }

        return map::sidecarPath(
            mapPath,
            "figure-" + std::to_string(figureIndex) + "-20x28.png");
    }

    void Editor::loadCharacterSkins()
    {
        ensurePlayerInRoster();

        std::vector<gfx::Bitmap> skinBitmaps;

        for (std::size_t index = 0; index < map.characters.size(); ++index)
        {
            gfx::Bitmap skinBitmap;

            try
            {
                skinBitmap = gfx::readPngFile(
                    characterSheetPath(index), kAppName);
            }
            catch (...)
            {
                skinBitmap = map::loadCharacterSheet(mapPath, kAppName);
            }

            if (skinBitmap.size != character::characterSheetSize())
            {
                skinBitmap = map::loadCharacterSheet(mapPath, kAppName);
            }

            skinBitmaps.push_back(std::move(skinBitmap));
        }

        characterView.takeSkins(viewportRenderer, std::move(skinBitmaps));
    }

    void Editor::saveCharacterSkins()
    {
        characterView.keepEdits(viewportRenderer);

        for (std::size_t index = 0;
             index < characterView.skins().size();
             ++index)
        {
            gfx::writePngFile(
                characterView.skins().at(index),
                characterSheetPath(index),
                kAppName);
        }
    }

    void Editor::pressFigure(
        const voxel::VoxelCell cell, const input::MouseButton button)
    {
        if (!figurePicked.has_value()
            || *figurePicked >= map.characters.size())
        {
            return;
        }

        auto &figure = map.characters.at(*figurePicked);

        if (button == input::MouseButton::Right)
        {
            pushUndo();

            if (!figure.patrolPathCells.empty() || figure.player)
            {
                figure.patrolPathCells.clear();

                return;
            }

            map.characters.erase(
                std::next(
                    map.characters.begin(),
                    static_cast<std::ptrdiff_t>(
                        *figurePicked)));
            figurePicked.reset();
            spawnRoster();
            loadCharacterSkins();

            return;
        }

        if (!figurePlaced)
        {
            const auto feet =
                (static_cast<float>(cell.y) + 0.5F)
                * voxel::kVoxelSide;
            const auto groundHeight =
                collision::groundHeightAtColumn(
                    worldMeshes.cells(), cell.x, cell.z, feet);

            if (!groundHeight.has_value())
            {
                return;
            }

            pushUndo();
            figure.idlePlacement = map::Placement{
                .position = antwika::gfx::Vec3{
                    static_cast<float>(cell.x) * voxel::kVoxelSide,
                    *groundHeight,
                    static_cast<float>(cell.z)
                        * voxel::kVoxelSide}};
            figurePlaced = true;

            return;
        }

        pushUndo();
        figure.patrolPathCells.push_back(
            voxel::VoxelCell{.x = cell.x, .y = cell.y, .z = cell.z});
    }

    std::vector<light::ActiveLight> Editor::currentLights()
    {
        if (!playing)
        {
            return light::activeLights(map.lamps);
        }

        const auto stoodPosition =
            game->world().get<component::Position>(game->player());
        const antwika::gfx::Vec3 walkerPosition{
            stoodPosition.x, stoodPosition.y, stoodPosition.z};
        const auto sightPoint =
            antwika::voxel::lineOfSight(walkerPosition);
        const auto upperSightPoint =
            antwika::voxel::upperLineOfSight(walkerPosition);

        std::vector<light::ActiveLight> lights{
            light::ActiveLight{.position = sightPoint},
            light::ActiveLight{.position = upperSightPoint}};

        for (const auto &lamp : light::activeLights(map.lamps))
        {
            if (lights.size() >= light::kMaxLamps)
            {
                break;
            }

            lights.push_back(lamp);
        }

        return lights;
    } // GCOVR_EXCL_LINE

    void Editor::sayCaption(
        const std::string &name,
        const std::string &line,
        const std::optional<std::size_t> speaker)
    {
        caption.name = name;
        caption.line = line;
        caption.speaker = speaker;
        caption.start = tick;
        caption.untilTick =
            tick + antwika::editor::kCaptionHoldTicks
            + (static_cast<std::uint32_t>(line.size())
               * antwika::editor::kCaptionCharTicks);
    }

    void Editor::ensurePlayerInRoster()
    {
        if (playerIndex(map).has_value())
        {
            return;
        }

        map.characters.push_back(
            map::Character{
                .name = "Player",
                .idlePlacement = startingPlacement(),
                .components = {"component::CarriedLight"},
                .player = true});
    }

    void Editor::spawnRoster()
    {
        game->forgetPatrols();
        ensurePlayerInRoster();
        patrolCells = patrolStopsOf(map);
        game->setPlayer(
            gameplay::spawnRoster(
                game->world(),
                map,
                *playerIndex(map),
                startingPlacement()));
    }

    void Editor::spawnItems()
    {
        gameplay::spawnItems(game->world(), map);
        game->world().commit();
    }

    void Editor::playApart()
    {
        saveCurrentMap();

        const auto assetFolder =
            (std::filesystem::path(io::assetPath(std::string(kAppName)))
                 .parent_path()
                 .parent_path()
             / "antwika_game" / "antwika_game")
                .string();

        if (!app::spawnDetached(assetFolder, {"--map", mapPath}))
        {
            showStatus("no game to play it with", true, 180);

            return;
        }

        showStatus("playing in a window of its own");
    }

    void Editor::drawSprite(
        const gfx::Camera3D &camera,
        const gfx::Mat4 &modelMatrix,
        gfx::ITexture *const sheetTexture,
        const component::Position stoodPosition,
        const component::AnimationState posedState)
    {
        sprites.drawCharacter(
            viewportRenderer,
            worldShader.program(),
            camera,
            modelMatrix,
            sheetTexture,
            stoodPosition,
            posedState,
            tick,
            lightPasses.lampShadows());
    }

    void Editor::interact()
    {
        const auto reveal =
            static_cast<std::uint32_t>(caption.line.size())
            * antwika::editor::kCaptionCharTicks;

        if (tick < caption.untilTick
            && tick - caption.start < reveal)
        {
            caption.start =
                tick > reveal ? tick - reveal : 0;
            caption.untilTick = tick + antwika::editor::kCaptionHoldTicks;

            return;
        }

        auto &world = game->world();
        const auto stoodPosition =
            world.get<component::Position>(game->player());

        for (const auto entity :
             world.view<component::Position, component::RosterIndex,
                 component::Speaker>())
        {
            if (world.has<component::Player>(entity))
            {
                continue;
            }

            const auto rosterIndex =
                world.get<component::RosterIndex>(entity).index;

            if (rosterIndex >= map.characters.size())
            {
                continue;
            }

            const auto therePosition = world.get<component::Position>(entity);
            const auto byX = therePosition.x - stoodPosition.x;
            const auto byZ = therePosition.z - stoodPosition.z;

            if ((byX * byX) + (byZ * byZ)
                > kTalkRadius * kTalkRadius)
            {
                continue;
            }

            const auto &figure = map.characters.at(rosterIndex);
            const auto speaker = world.get<component::Speaker>(entity);

            sayCaption(
                figure.name.empty()
                    ? "figure " + std::to_string(rosterIndex)
                    : figure.name,
                figure.dialogue.at(
                    speaker.nextLineIndex % figure.dialogue.size()),
                rosterIndex);
            world.set<component::Speaker>(
                entity,
                component::Speaker{
                    .nextLineIndex = speaker.nextLineIndex + 1});
            world.commit();

            return;
        }
    }

}
