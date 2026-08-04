#pragma once

#include <functional>
#include <optional>
#include <string>

namespace antwika::app
{

    /**
     * @brief Offer a store to a live run and withhold it from a replay.
     *
     * **This is the guard that keeps a replay from writing.** Reading
     * the machine's file would resolve a recorded session against a
     * state the recording never carried, and writing it would leave
     * whoever replayed somebody else's session carrying its result --
     * a high score they never made, a companion they never raised.
     *
     * Two applications wrote this out identically, eight lines each,
     * and their header docs even shared the sentence about no main()
     * having to make the decision. It is one rule, so it is stated
     * once, where every application can reach it and where it has a
     * test of its own.
     *
     * @tparam StoreT The store interface the application keeps.
     * @param store The machine's store.
     * @param replayPath What --replay named, if anything.
     * @return The store for a live run; nothing at all for a replay.
     */
    template <typename StoreT>
    [[nodiscard]] std::optional<std::reference_wrapper<StoreT>> storeIfLive(
        StoreT &store, const std::optional<std::string> &replayPath)
    {
        if (replayPath.has_value())
        {
            return std::nullopt;
        }

        return store;
    }

} // namespace antwika::app
