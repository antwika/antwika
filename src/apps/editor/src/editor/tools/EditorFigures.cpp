#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

#include <antwika/io/AssetPath.hpp>
#include <antwika/gfx/PngFile.hpp>
#include <antwika/app/SpawnDetached.hpp>
#include <antwika/component/AnimationState.hpp>
#include <antwika/component/DialogueLine.hpp>
#include <antwika/component/TalkIntent.hpp>
#include <antwika/ecs/OpenPhase.hpp>
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
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/VoxelOcclusion.hpp>

#include "antwika/editor/Editor.hpp"

namespace
{

    const std::vector<std::string> kPlayerComponents{
        "component::Position",
        "component::Velocity",
        "component::AnimationState",
        "component::RosterIndex",
        "component::Health",
        "component::Inventory",
        "component::Player",
        "component::FillLight",
        "component::CarriedLight"};

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
            const auto sheetPath = characterSheetPath(index);
            std::error_code errorCode;

            std::filesystem::create_directories(
                std::filesystem::path(sheetPath).parent_path(), errorCode);
            gfx::writePngFile(
                characterView.skins().at(index), sheetPath, kAppName);
        }
    }

    void Editor::pressFigure(
        const voxel::VoxelPosition position, const input::MouseButton button)
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

            if (!figure.patrolPathPositions.empty() || figure.player)
            {
                figure.patrolPathPositions.clear();

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
                (static_cast<float>(position.y) + 0.5F)
                * voxel::kVoxelSide;
            const auto groundHeight =
                collision::groundHeightAtColumn(
                    worldMeshes.cells(), position.x, position.z, feet);

            if (!groundHeight.has_value())
            {
                return;
            }

            pushUndo();
            figure.idlePlacement = map::Placement{
                .position = antwika::gfx::Vec3{
                    static_cast<float>(position.x) * voxel::kVoxelSide,
                    *groundHeight,
                    static_cast<float>(position.z)
                        * voxel::kVoxelSide}};
            figurePlaced = true;

            return;
        }

        pushUndo();
        figure.patrolPathPositions.push_back(
            voxel::VoxelPosition{.x = position.x, .y = position.y,
                .z = position.z});
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
                .components = kPlayerComponents,
                .player = true});
    }

    void Editor::spawnRoster()
    {
        game->forgetPatrols();
        ensurePlayerInRoster();
        patrolPositions = patrolStopsOf(map);
        game->setPlayer(
            gameplay::spawnRoster(
                game->world(),
                map,
                *playerIndex(map),
                startingPlacement()));
    }

    void Editor::spawnItems()
    {
        const ecs::OpenPhase phase(game->world());

        gameplay::spawnItems(game->world(), map);
    }

    void Editor::playApart()
    {
        saveCurrentMap();

        if (mapPath.empty())
        {
            return;
        }

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
        const ecs::OpenPhase phase(world);

        world.add<component::TalkIntent>(
            game->player(), component::TalkIntent{});
    }

    void Editor::sayDialogueLine()
    {
        auto &world = game->world();

        if (!world.has<component::DialogueLine>(game->player()))
        {
            return;
        }

        const auto dialogueLine =
            world.get<component::DialogueLine>(game->player());

        if (dialogueLine.rosterIndex < map.characters.size())
        {
            const auto &figure = map.characters.at(dialogueLine.rosterIndex);

            if (!figure.dialogue.empty())
            {
                sayCaption(
                    figure.name.empty()
                        ? "figure " + std::to_string(dialogueLine.rosterIndex)
                        : figure.name,
                    figure.dialogue.at(
                        dialogueLine.lineIndex % figure.dialogue.size()),
                    dialogueLine.rosterIndex);
            }
        }

        const ecs::OpenPhase phase(world);

        world.remove<component::DialogueLine>(game->player());
    }

}
