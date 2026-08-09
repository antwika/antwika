#pragma once

#include <stdexcept>

namespace antwika::ui
{

    class UiError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

}
