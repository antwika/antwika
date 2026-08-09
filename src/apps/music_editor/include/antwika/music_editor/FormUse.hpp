#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/pattern/Span.hpp>

namespace antwika::music_editor
{

    struct FormUse final
    {
        std::string name;

        std::int64_t bars = 0;

        [[nodiscard]] bool operator==(const FormUse &other) const
            = default;
    };

    [[nodiscard]] std::vector<FormUse> readFormLine(
        std::string_view text);

    [[nodiscard]] std::int64_t readBarsLine(std::string_view text);

    [[nodiscard]] std::vector<std::string> readPartLine(
        std::string_view text);

    void resolveBars(std::vector<FormUse> &uses, std::int64_t fallback);

    [[nodiscard]] std::int64_t periodOf(
        const std::vector<FormUse> &uses) noexcept;

    [[nodiscard]] std::vector<pattern::Span> windowsFor(
        const std::vector<std::string> &parts,
        const std::vector<FormUse> &uses);

}
