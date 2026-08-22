#include "antwika/system/TalkSystem.hpp"

#include <antwika/component/DialogueLine.hpp>
#include <antwika/component/Player.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/component/RosterIndex.hpp>
#include <antwika/component/Speaker.hpp>
#include <antwika/component/TalkIntent.hpp>

namespace antwika::system
{

    void TalkSystem::setRosterCount(const std::size_t value) noexcept
    {
        rosterCount = value;
    }

    void TalkSystem::update(ecs::World &world, const time::Tick)
    {
        for (const auto talker :
             world.view<component::TalkIntent, component::Position>())
        {
            const auto stoodPosition =
                world.get<component::Position>(talker);

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

                if (rosterIndex >= rosterCount)
                {
                    continue;
                }

                const auto therePosition =
                    world.get<component::Position>(entity);
                const auto byX = therePosition.x - stoodPosition.x;
                const auto byZ = therePosition.z - stoodPosition.z;

                if ((byX * byX) + (byZ * byZ) > kTalkRadius * kTalkRadius)
                {
                    continue;
                }

                const auto speaker = world.get<component::Speaker>(entity);

                world.set<component::Speaker>(
                    entity,
                    component::Speaker{
                        .nextLineIndex = speaker.nextLineIndex + 1});
                world.add<component::DialogueLine>(
                    talker,
                    component::DialogueLine{
                        .rosterIndex = rosterIndex,
                        .lineIndex = speaker.nextLineIndex});

                break;
            }

            world.remove<component::TalkIntent>(talker);
        }
    }

}
