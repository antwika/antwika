#include "antwika/tilemap/Column.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <utility>

namespace antwika::tilemap
{

    namespace
    {
        [[nodiscard]] bool slabBelow(
            const Slab &slab, const std::int32_t level) noexcept
        {
            return slab.level < level;
        }

        [[nodiscard]] bool levelBelow(
            const std::int32_t level, const Slab &slab) noexcept
        {
            return level < slab.level;
        }
    }

    const std::vector<Slab> &Column::slabs() const noexcept
    {
        return slabs_;
    }

    Slab *Column::slabAt(const std::int32_t level) noexcept
    {
        return const_cast<Slab *>(std::as_const(*this).slabAt(level));
    }

    const Slab *Column::slabAt(
        const std::int32_t level) const noexcept
    {
        const auto it = std::lower_bound(
            slabs_.begin(), slabs_.end(), level, slabBelow);

        if (it == slabs_.end() || it->level != level)
        {
            return nullptr;
        }

        return &*it;
    }

    Slab *Column::top() noexcept
    {
        return const_cast<Slab *>(std::as_const(*this).top());
    }

    const Slab *Column::top() const noexcept
    {
        return slabs_.empty() ? nullptr : &slabs_.back();
    }

    const Slab *Column::topAtOrBelow(
        const std::int32_t level) const noexcept
    {
        const auto it = std::upper_bound(
            slabs_.begin(), slabs_.end(), level, levelBelow);

        if (it == slabs_.begin())
        {
            return nullptr;
        }

        return &*std::prev(it);
    }

    bool Column::standable(const std::int32_t level) const noexcept
    {
        const auto *slab = slabAt(level);

        if (slab == nullptr)
        {
            return false;
        }

        const auto next =
            static_cast<std::size_t>(slab - slabs_.data()) + 1;

        if (next == slabs_.size())
        {
            return true;
        }

        return slabs_[next].level > std::int64_t{level} + kClearance;
    }

    Slab &Column::place(Slab slab)
    {
        const auto it = std::lower_bound(
            slabs_.begin(), slabs_.end(), slab.level, slabBelow);

        if (it != slabs_.end() && it->level == slab.level)
        {
            *it = slab;
            return *it;
        }

        return *slabs_.insert(it, slab);
    }

    bool Column::remove(const std::int32_t level)
    {
        const auto it = std::lower_bound(
            slabs_.begin(), slabs_.end(), level, slabBelow);

        if (it == slabs_.end() || it->level != level)
        {
            return false;
        }

        slabs_.erase(it);

        return true;
    }

    void Column::clear() noexcept
    {
        slabs_.clear();
    }

}
