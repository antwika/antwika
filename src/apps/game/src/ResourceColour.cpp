#include "antwika/game/ResourceColour.hpp"

#include <array>

namespace antwika::game
{

    namespace
    {
        constexpr Color kFoodInk{
            .red = 126, .green = 196, .blue = 84};

        constexpr Color kClayInk{
            .red = 178, .green = 122, .blue = 78};

        constexpr Color kPotteryInk{
            .red = 210, .green = 168, .blue = 96};

        constexpr Color kWaterInk{
            .red = 104, .green = 174, .blue = 216};

        constexpr Color kHealthInk{
            .red = 214, .green = 120, .blue = 148};

        constexpr Color kSafetyInk{
            .red = 224, .green = 148, .blue = 78};

        constexpr Color kStructureInk{
            .red = 170, .green = 176, .blue = 188};
    } // namespace

    Color resourceColour(Resource resource) noexcept
    {
        constexpr std::array<Color, kResourceCount> inks{
            kFoodInk,
            kClayInk,
            kPotteryInk};

        return inks[resourceIndex(resource) % kResourceCount];
    }

    Color serviceColour(Service service) noexcept
    {
        constexpr std::array<Color, kServiceCount> inks{
            kWaterInk,
            kHealthInk,
            kSafetyInk,
            kStructureInk};

        return inks[serviceIndex(service) % kServiceCount];
    }

} // namespace antwika::game
