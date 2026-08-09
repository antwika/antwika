#pragma once

#include <functional>
#include <optional>
#include <string>

namespace antwika::app
{

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

}
