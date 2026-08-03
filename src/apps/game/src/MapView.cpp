#include "antwika/game/MapView.hpp"

namespace antwika::game
{

    MapView MapViewState::view() const noexcept
    {
        return showing;
    }

    void MapViewState::set(MapView wanted) noexcept
    {
        showing = wanted;
    }

} // namespace antwika::game
