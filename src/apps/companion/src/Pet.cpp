#include "antwika/companion/Pet.hpp"

#include <string>
#include <string_view>

#include "antwika/companion/CompanionError.hpp"

namespace antwika::companion
{

    namespace
    {
        // One check called eight times rather than eight checks.
        // A conjunction would be eight branches to reach both ways.
        // This is one branch, reached both ways by two tests.
        void requirePositive(const Tick value, const std::string_view what)
        {
            if (value != 0)
            {
                return;
            }

            throw CompanionError(
                "companion: " + std::string(what) + " must not be zero");
        }

        PetConfig validated(const PetConfig config)
        {
            requirePositive(config.dayTicks, "dayTicks");
            requirePositive(config.nightTicks, "nightTicks");
            requirePositive(config.hungerPeriodTicks, "hungerPeriodTicks");
            requirePositive(config.starvePeriodTicks, "starvePeriodTicks");
            requirePositive(config.restPeriodTicks, "restPeriodTicks");
            requirePositive(config.hungerMax, "hungerMax");
            requirePositive(config.happinessMax, "happinessMax");
            requirePositive(config.happinessStart, "happinessStart");

            return config;
        }
    } // namespace

    Pet::Pet(const PetConfig config)
        : config(validated(config)),
          happinessLevel(this->config.happinessStart)
    {
    }

    void Pet::step()
    {
        // The clock runs whatever has happened to the companion.
        // So the picture keeps animating rather than freezing.
        // A grave does not move, but the day over it still does.
        ++elapsed;

        if (petState == PetState::Perished)
        {
            return;
        }

        // Falling asleep and waking up are the clock and nothing else.
        // So neither can get out of step with the day it follows.
        const bool isNight = night();
        petState = isNight ? PetState::Asleep : PetState::Awake;

        // Cleared on the first tick of a night rather than at dawn.
        // A tap in the daylight must not spoil the night after it.
        if (elapsed % cycleTicks() == config.dayTicks)
        {
            disturbedTonight = false;
        }

        if (!isNight)
        {
            if (elapsed % config.hungerPeriodTicks == 0
                && hungerLevel < config.hungerMax)
            {
                ++hungerLevel;
            }

            // Famished is the violation, and merely hungry is not.
            // The gap between the two is how much warning there is.
            if (hungerLevel >= config.hungerMax
                && elapsed % config.starvePeriodTicks == 0)
            {
                lose(1);
            }

            return;
        }

        if (!disturbedTonight && elapsed % config.restPeriodTicks == 0)
        {
            gain(1);
        }
    }

    void Pet::tap()
    {
        if (petState == PetState::Perished)
        {
            return;
        }

        if (petState == PetState::Asleep)
        {
            disturbedTonight = true;
            ++disturbanceCount;
            lose(config.disturbCost);
            return;
        }

        // Food a companion does not want is left uneaten.
        // It is neither the right action nor a violation of either need.
        // So nothing at all comes of it.
        // That is what keeps tapping repeatedly from being a strategy.
        if (!hungry())
        {
            return;
        }

        hungerLevel = hungerLevel > config.feedRelief
                          ? hungerLevel - config.feedRelief
                          : 0;
        ++mealCount;
        gain(config.feedJoy);
    }

    PetState Pet::state() const noexcept
    {
        return petState;
    }

    std::uint32_t Pet::hunger() const noexcept
    {
        return hungerLevel;
    }

    std::uint32_t Pet::happiness() const noexcept
    {
        return happinessLevel;
    }

    bool Pet::hungry() const noexcept
    {
        return hungerLevel >= config.hungerThreshold;
    }

    bool Pet::night() const noexcept
    {
        return elapsed % cycleTicks() >= config.dayTicks;
    }

    bool Pet::disturbed() const noexcept
    {
        return disturbedTonight;
    }

    Tick Pet::ticks() const noexcept
    {
        return elapsed;
    }

    std::uint32_t Pet::meals() const noexcept
    {
        return mealCount;
    }

    std::uint32_t Pet::disturbances() const noexcept
    {
        return disturbanceCount;
    }

    const PetConfig &Pet::settings() const noexcept
    {
        return config;
    }

    Tick Pet::cycleTicks() const noexcept
    {
        return config.dayTicks + config.nightTicks;
    }

    void Pet::gain(const std::uint32_t amount) noexcept
    {
        happinessLevel += amount;

        if (happinessLevel > config.happinessMax)
        {
            happinessLevel = config.happinessMax;
        }
    }

    void Pet::lose(const std::uint32_t amount) noexcept
    {
        happinessLevel =
            happinessLevel > amount ? happinessLevel - amount : 0;

        // Perishing is where the two needs meet.
        // Neglect and disturbance spend the same number.
        // So either of them can be what runs it out.
        if (happinessLevel == 0)
        {
            petState = PetState::Perished;
        }
    }

} // namespace antwika::companion
