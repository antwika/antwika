#include "antwika/ecs/OpenPhase.hpp"

#include <exception>
#include <string>
#include <utility>

#include <antwika/log/Level.hpp>

namespace antwika::ecs
{

    OpenPhase::OpenPhase(World &world) noexcept : world(&world)
    {
    }

    OpenPhase::~OpenPhase()
    {
        if (world == nullptr)
        {
            return;
        }

        auto &logger = world->getLogger();

        try
        {
            close();
        }
        catch (const std::exception &error)
        {
            logger.log(
                log::Level::Error,
                std::string("OpenPhase: a commit failed while closing: ")
                    + error.what());
        }
    }

    void OpenPhase::close()
    {
        auto *const closingWorld = std::exchange(world, nullptr);

        if (closingWorld != nullptr)
        {
            closingWorld->commit();
        }
    }

}
