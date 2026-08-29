#include "antwika/editor/editor/SimulationSteps.hpp"

#include <algorithm>
#include <string>

#include <antwika/collision/Collision.hpp>
#include <antwika/component/AnimationState.hpp>
#include <antwika/component/ConsumeIntent.hpp>
#include <antwika/component/ConsumeReport.hpp>
#include <antwika/component/DialogueLine.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/component/TalkIntent.hpp>
#include <antwika/ecs/OpenPhase.hpp>
#include <antwika/editor/ui/EditorLook.hpp>
#include <antwika/gameplay/CheckpointState.hpp>
#include <antwika/gameplay/PadReports.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxelmap/Voxel.hpp>

namespace antwika::editor
{

    SimulationSteps::SimulationSteps(
        EditorDocument &document,
        IEditSteps &editSteps,
        const std::uint32_t &tick) noexcept
        : document(document), editSteps(editSteps), tick(tick)
    {
    }

    void SimulationSteps::consumeItem(
        gameplay::IWorldAccess &worldAccess, const component::ItemKind kind)
    {
        auto &gameWorld = worldAccess.getWorld();
        const ecs::OpenPhase phase(gameWorld);

        gameWorld.add<component::ConsumeIntent>(
            worldAccess.getPlayer(),
            component::ConsumeIntent{
                .kind = static_cast<std::uint8_t>(kind)});
    }

    void SimulationSteps::sayConsumeReport(gameplay::IWorldAccess &worldAccess)
    {
        auto &gameWorld = worldAccess.getWorld();

        if (!gameWorld.has<component::ConsumeReport>(worldAccess.getPlayer()))
        {
            return;
        }

        const auto report =
            gameWorld.get<component::ConsumeReport>(worldAccess.getPlayer());
        const auto kind = static_cast<component::ItemKind>(report.kind);

        sayCaption(
            kind == component::ItemKind::Food ? "food" : "water",
            report.anyLeft
                ? (kind == component::ItemKind::Food ? "eaten" : "drunk")
                : "there is none left to take");

        const ecs::OpenPhase phase(gameWorld);

        gameWorld.remove<component::ConsumeReport>(worldAccess.getPlayer());
    }

    void SimulationSteps::sayCheckpointReport(
        gameplay::ICheckpointProgress &checkpointProgress,
        gameplay::IWorldAccess &worldAccess)
    {
        if (gameplay::takeCheckpointReport(
                checkpointProgress,
                worldAccess.getWorld(),
                worldAccess.getPlayer()))
        {
            sayCaption("checkpoint", "the respawn is set here");
        }
    }

    void SimulationSteps::interact(gameplay::IWorldAccess &worldAccess)
    {
        const auto reveal =
            static_cast<std::uint32_t>(caption.line.size())
            * kCaptionCharTicks;

        if (tick < caption.untilTick
            && tick - caption.start < reveal)
        {
            caption.start =
                tick > reveal ? tick - reveal : 0;
            caption.untilTick = tick + kCaptionHoldTicks;

            return;
        }

        auto &gameWorld = worldAccess.getWorld();
        const ecs::OpenPhase phase(gameWorld);

        gameWorld.add<component::TalkIntent>(
            worldAccess.getPlayer(), component::TalkIntent{});
    }

    void SimulationSteps::sayDialogueLine(gameplay::IWorldAccess &worldAccess)
    {
        auto &gameWorld = worldAccess.getWorld();

        if (!gameWorld.has<component::DialogueLine>(worldAccess.getPlayer()))
        {
            return;
        }

        const auto dialogueLine =
            gameWorld.get<component::DialogueLine>(worldAccess.getPlayer());

        if (dialogueLine.characterIndex < document.map.characters.size())
        {
            const auto &character = document.map.characters.at(
                dialogueLine.characterIndex);

            if (!character.dialogue.empty())
            {
                sayCaption(
                    character.name.empty()
                        ? "character "
                              + std::to_string(
                                  dialogueLine.characterIndex)
                        : character.name,
                    character.dialogue.at(
                        dialogueLine.lineIndex % character.dialogue.size()),
                    dialogueLine.characterIndex);
            }
        }

        const ecs::OpenPhase phase(gameWorld);

        gameWorld.remove<component::DialogueLine>(worldAccess.getPlayer());
    }

    void SimulationSteps::sayCaption(
        const std::string &name,
        const std::string &line,
        const std::optional<std::size_t> speaker)
    {
        caption.name = name;
        caption.line = line;
        caption.speaker = speaker;
        caption.start = tick;
        caption.untilTick =
            tick + kCaptionHoldTicks
            + (static_cast<std::uint32_t>(line.size()) * kCaptionCharTicks);
    }

}
