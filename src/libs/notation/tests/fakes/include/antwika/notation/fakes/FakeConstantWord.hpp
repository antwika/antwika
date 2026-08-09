#pragma once

#include <cstddef>
#include <string_view>

#include <antwika/pattern/Controls.hpp>
#include <antwika/pattern/ParamId.hpp>
#include <antwika/pattern/ParamValue.hpp>

#include "antwika/notation/IWordReader.hpp"

namespace antwika::notation::fakes
{

    class FakeConstantWord final : public IWordReader
    {
    public:
        explicit FakeConstantWord(pattern::ParamId name) : name(name)
        {
        }

        [[nodiscard]] pattern::Controls read(
            std::string_view, std::size_t) const override
        {
            return pattern::Controls(name, pattern::ParamValue(1));
        }

    private:
        pattern::ParamId name;
    };

}
