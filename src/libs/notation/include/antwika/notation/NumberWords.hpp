#pragma once

#include <cstddef>
#include <string_view>

#include <antwika/pattern/Controls.hpp>
#include <antwika/pattern/ParamId.hpp>

#include "antwika/notation/IWordReader.hpp"

namespace antwika::notation
{

    using antwika::pattern::ParamId;

    class NumberWords final : public IWordReader
    {
    public:
        explicit NumberWords(ParamId id) noexcept;

        [[nodiscard]] Controls read(
            std::string_view word, std::size_t at) const override;

    private:
        ParamId named;
    };

}
