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

        // The medicine's ink is kDiseaseRiskInk on purpose.
        // Medicine is what holds the disease off.
        // So the amount line and its risk line read as a pair.
        constexpr Color kMedicineInk = kDiseaseRiskInk;
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
            kMedicineInk};

        return inks[serviceIndex(service) % kServiceCount];
    }

} // namespace antwika::game
