#include "antwika/ecs/ComponentId.hpp"

namespace antwika::ecs::detail
{

    std::size_t nextComponentId() noexcept
    {
        static std::size_t next = 0;
        return next++;
    }

}
