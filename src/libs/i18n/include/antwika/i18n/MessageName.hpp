#pragma once

#include <string_view>

namespace antwika::i18n
{

    template <typename Id>
    struct MessageName final
    {
        Id id{};

        std::string_view name;
    };

}
