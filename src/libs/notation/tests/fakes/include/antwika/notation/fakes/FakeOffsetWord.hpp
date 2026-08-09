#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include <antwika/pattern/Controls.hpp>
#include <antwika/pattern/ParamId.hpp>
#include <antwika/pattern/ParamValue.hpp>

#include "antwika/notation/IWordReader.hpp"

namespace antwika::notation::fakes
{

    class FakeOffsetWord final : public IWordReader
    {
    public:
        explicit FakeOffsetWord(pattern::ParamId name) : name(name)
        {
        }

        [[nodiscard]] pattern::Controls read(
            std::string_view, std::size_t at) const override
        {
            return pattern::Controls(
                name,
                pattern::ParamValue(static_cast<std::int64_t>(at)));
        }

    private:
        pattern::ParamId name;
    };

}
