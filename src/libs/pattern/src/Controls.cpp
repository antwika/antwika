#include "antwika/pattern/Controls.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <vector>

#include "antwika/pattern/ParamId.hpp"
#include "antwika/pattern/ParamValue.hpp"

namespace antwika::pattern
{

    namespace
    {
        [[nodiscard]] auto findSlot(
            std::vector<Control> &entries, ParamId id)
        {
            return std::ranges::lower_bound(
                entries,
                id,
                std::ranges::less{},
                [](const Control &entry) { return entry.id; });
        }
    }

    Controls::Controls(ParamId id, ParamValue value)
    {
        set(id, value);
    }

    void Controls::set(ParamId id, ParamValue value)
    {
        const auto slot = findSlot(entries, id);

        if (slot != entries.end() && slot->id == id)
        {
            slot->value = value;

            return;
        }

        entries.insert(slot, Control{.id = id, .value = value});
    }

    std::optional<ParamValue> Controls::get(ParamId id) const
    {
        const auto slot = std::ranges::lower_bound(
            entries,
            id,
            std::ranges::less{},
            [](const Control &entry) { return entry.id; });

        if (slot == entries.end() || slot->id != id)
        {
            return std::nullopt;
        }

        return slot->value;
    }

    std::size_t Controls::size() const noexcept
    {
        return entries.size();
    }

    bool Controls::empty() const noexcept
    {
        return entries.empty();
    }

    const std::vector<Control> &Controls::all() const noexcept
    {
        return entries;
    }

    Controls Controls::mergedWith(const Controls &over) const
    {
        Controls combined = *this;

        for (const auto &entry : over.entries)
        {
            combined.set(entry.id, entry.value);
        }

        return combined;

    } // GCOVR_EXCL_LINE

}
