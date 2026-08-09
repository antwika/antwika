#include "antwika/game/ResourceColour.hpp"

#include <array>

#include <antwika/enums/Enumeration.hpp>

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

        constexpr Color kMedicineInk = kDiseaseRiskInk;
    }

    Color resourceColour(Resource resource) noexcept
    {
        constexpr std::array<Color, kResourceCount> inks{
            kFoodInk,
            kClayInk,
            kPotteryInk};

        return antwika::enums::pick(inks, resource);
    }

    Color serviceColour(Service service) noexcept
    {
        constexpr std::array<Color, kServiceCount> inks{
            kWaterInk,
            kMedicineInk};

        return antwika::enums::pick(inks, service);
    }

}
