#pragma once

#include "antwika/component/Health.hpp"
#include "antwika/component/Inventory.hpp"

namespace antwika::component
{

    struct Vitals final
    {
        Health health{};

        Inventory inventory{};
    };

}
