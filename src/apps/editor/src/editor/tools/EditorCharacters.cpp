#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

#include <antwika/io/AssetPath.hpp>
#include <antwika/image/PngFile.hpp>
#include <antwika/app/SpawnDetached.hpp>
#include <antwika/component/AnimationState.hpp>
#include <antwika/ecs/OpenPhase.hpp>
#include <antwika/component/Player.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/component/CharacterIndex.hpp>
#include <antwika/component/Speaker.hpp>
#include <antwika/editor/ui/EditorLook.hpp>
#include <antwika/gfx/Camera3D.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/MeshMaterial.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/assets/MapAssets.hpp>
#include <antwika/gameplay/ComponentNames.hpp>
#include <antwika/gameplay/Characters.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/VoxelOcclusion.hpp>

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    std::string Editor::getCharacterSheetPath(
        const std::size_t characterIndex) const
    {
        if (characterIndex < document.map.characters.size()
            && document.map.characters.at(characterIndex).player)
        {
            return map::getSharedTexturePath(document.getPath(),
                character::kCharacterSheet);
        }

        return map::getSidecarPath(
            document.getPath(),
            "character-" + std::to_string(characterIndex)
                + "-20x28.png");
    }

    void Editor::loadCharacterSkins()
    {
        ensurePlayerCharacter();

        std::vector<gfx::Bitmap> skinBitmaps;

        for (std::size_t index = 0; index < document.map.characters.size(
                ); ++index)
        {
            gfx::Bitmap skinBitmap;

            try
            {
                skinBitmap = image::getReadPngFile(
                    getCharacterSheetPath(index), kAppName);
            }
            catch (...)
            {
                skinBitmap = assets::getLoadCharacterSheet(document.getPath(), kAppName);
            }

            if (skinBitmap.size != character::getCharacterSheetSize())
            {
                skinBitmap = assets::getLoadCharacterSheet(document.getPath(), kAppName);
            }

            skinBitmaps.push_back(std::move(skinBitmap));
        }

        characterView.takeSkins(viewportRenderer, characterSkins, std::move(skinBitmaps));
    }

    void Editor::saveCharacterSkins()
    {
        characterView.keepEdits(viewportRenderer, characterSkins);

        for (std::size_t index = 0;
             index < characterSkins.getSheets().size();
             ++index)
        {
            const auto sheetPath = getCharacterSheetPath(index);
            std::error_code errorCode;

            std::filesystem::create_directories(
                std::filesystem::path(sheetPath).parent_path(), errorCode);
            image::writePngFile(
                characterSkins.getSheets().at(index), sheetPath, kAppName);
        }
    }

    void Editor::pressCharacter(
        const voxel::VoxelPosition position, const input::MouseButton button)
    {
        const auto chosenCharacter = worldView.characterTool().getChosenCharacter(
            document.map.characters.size());

        if (!chosenCharacter.has_value())
        {
            return;
        }

        auto &character = document.map.characters.at(*chosenCharacter);

        if (button == input::MouseButton::Right)
        {
            pushUndo();

            if (!character.patrolPathPositions.empty() || character.player)
            {
                character.patrolPathPositions.clear();

                return;
            }

            document.map.characters.erase(
                std::next(
                    document.map.characters.begin(),
                    static_cast<std::ptrdiff_t>(*chosenCharacter)));
            worldView.characterTool().dropChoice();
            spawnCharacters();
            loadCharacterSkins();

            return;
        }

        if (!worldView.characterTool().isPlaced())
        {
            const auto feet =
                (static_cast<float>(position.y) + 0.5F)
                * voxel::kVoxelSide;
            const auto groundHeight =
                collision::getGroundHeightAtColumn(
                    worldMeshes.getCells(), position.x, position.z, feet);

            if (!groundHeight.has_value())
            {
                return;
            }

            pushUndo();
            character.idlePlacement = map::Placement{
                .position = antwika::gfx::Vec3{
                    static_cast<float>(position.x) * voxel::kVoxelSide,
                    *groundHeight,
                    static_cast<float>(position.z)
                        * voxel::kVoxelSide}};
            worldView.characterTool().markPlaced();

            return;
        }

        pushUndo();
        character.patrolPathPositions.push_back(
            voxel::VoxelPosition{.x = position.x, .y = position.y,
                .z = position.z});
    }

    std::vector<light::ActiveLight> Editor::currentLights()
    {
        if (!play.playing)
        {
            return light::getActiveLights(document.map.lamps);
        }

        const auto stoodPosition =
            play.game->getWorld().get<component::Position>(play.game->getPlayer());
        const antwika::gfx::Vec3 walkerPosition{
            stoodPosition.x, stoodPosition.y, stoodPosition.z};
        const auto sightPoint =
            antwika::voxel::getLineOfSight(walkerPosition);
        const auto upperSightPoint =
            antwika::voxel::getUpperLineOfSight(walkerPosition);

        std::vector<light::ActiveLight> lights{
            light::ActiveLight{
                .position = sightPoint, .brightness = 0.0F}};

        if (worldView.isUpperSightOn(viewContextNow()))
        {
            lights.push_back(
                light::ActiveLight{
                    .position = upperSightPoint, .brightness = 0.0F});
        }

        for (const auto &lamp : light::getActiveLights(document.map.lamps))
        {
            if (lights.size() >= light::kMaxLamps)
            {
                break;
            }

            lights.push_back(lamp);
        }

        return lights;
    } // GCOVR_EXCL_LINE

    void Editor::ensurePlayerCharacter()
    {
        if (getPlayerIndex(document.map).has_value())
        {
            return;
        }

        const auto playerNames = gameplay::getPlayerComponentNames();

        document.map.characters.push_back(
            map::Character{
                .name = "Player",
                .idlePlacement = startingPlacement(),
                .components = std::vector<std::string>(
                    playerNames.begin(), playerNames.end()),
                .player = true});
    }

    void Editor::spawnCharacters()
    {
        play.game->forgetPatrols();
        ensurePlayerCharacter();
        play.patrolPositions = patrolStopsOf(document.map);
        gameplay::spawnCharacters(
            play.game->getWorld(),
            document.map,
            *getPlayerIndex(document.map));
        play.game->standPlayer();
    }

    void Editor::spawnItems()
    {
        const ecs::OpenPhase phase(play.game->getWorld());

        gameplay::spawnItems(play.game->getWorld(), document.map);
    }

    void Editor::playApart()
    {
        saveCurrentMap();

        if (document.getPath().empty())
        {
            return;
        }

        const auto assetFolder =
            (std::filesystem::path(io::getAssetPath(std::string(kAppName)))
                 .parent_path()
                 .parent_path()
             / "antwika_game" / "antwika_game")
                .string();

        if (!app::isSpawnDetached(assetFolder, {"--map", document.getPath()}))
        {
            showStatus("no game to play it with", true, 180);

            return;
        }

        showStatus("playing in a window of its own");
    }

}
