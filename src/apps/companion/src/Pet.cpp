#include "antwika/companion/Pet.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "antwika/companion/CompanionError.hpp"
#include "antwika/companion/SaveFormatError.hpp"

namespace antwika::companion
{

    namespace
    {
        // What it says when nothing in particular is happening.
        // Four of them, so a session is not one line on repeat.
        constexpr std::array<Saying, 4> kIdleSayings{
            Saying::Hello,
            Saying::Bored,
            Saying::NiceDay,
            Saying::Silly};

        // Which idle line comes up is a hash of the tick it comes up on.
        // A generator would be a seed and a position for a save to carry.
        // One let out of step would say wrong things for a whole session.
        // A plain (tick / period) % count reads as a carousel instead.
        // It is the same four lines in the same order, forever.
        // A hash is neither: no state at all, and no order to notice.
        // It is a pure function of the one number Pet already is one of.
        // The arithmetic is the murmur3 finalizer over exact widths.
        // So which line comes up is the same on every toolchain.
        [[nodiscard]] std::size_t idleIndex(const Tick tick) noexcept
        {
            auto mixed = static_cast<std::uint64_t>(tick);
            mixed ^= mixed >> 33;
            mixed *= 0xff51afd7ed558ccdULL;
            mixed ^= mixed >> 33;

            return static_cast<std::size_t>(mixed % kIdleSayings.size());
        }

        // One check called ten times rather than ten checks.
        // A conjunction would be ten branches to reach both ways.
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
            requirePositive(config.sayingTicks, "sayingTicks");
            requirePositive(
                config.chatterPeriodTicks, "chatterPeriodTicks");
            requirePositive(config.hungerMax, "hungerMax");
            requirePositive(config.happinessMax, "happinessMax");
            requirePositive(config.happinessStart, "happinessStart");

            return config;
        }

        // What a live companion could never be, refused not repaired.
        // A ceiling raised by a file is a gauge past its own end.
        // And Perished is exactly "the happiness ran out", both ways.
        // lose() alone reaches zero, and it sets the state as it does.
        // Nothing ever gives a perished companion any back.
        // So a file claiming one without the other is not this build's.
        void requireLivable(
            const PetConfig &config, const PetMemory &memory)
        {
            if (memory.hunger > config.hungerMax)
            {
                throw SaveFormatError(
                    "companion: a saved companion is hungrier than it "
                    "can be");
            }

            if (memory.happiness > config.happinessMax)
            {
                throw SaveFormatError(
                    "companion: a saved companion is happier than it "
                    "can be");
            }

            if ((memory.happiness == 0)
                != (memory.state == PetState::Perished))
            {
                throw SaveFormatError(
                    "companion: a saved companion has perished with "
                    "happiness left, or has none and lives on");
            }
        }
    } // namespace

    Pet::Pet(const PetConfig config)
        : config(validated(config)),
          happinessLevel(this->config.happinessStart)
    {
    }

    // Delegating rather than a second initialiser list.
    // The configuration is checked in exactly one place that way.
    // And a field added to Pet cannot be forgotten by one of two.
    Pet::Pet(const PetConfig config, const PetMemory &memory)
        : Pet(config)
    {
        requireLivable(this->config, memory);

        petState = memory.state;
        said = memory.saying;
        sayingLeft = memory.sayingTicksLeft;
        elapsed = memory.ticks;
        hungerLevel = memory.hunger;
        happinessLevel = memory.happiness;
        mealCount = memory.meals;
        disturbanceCount = memory.disturbances;
        pesterCount = memory.pesters;
        disturbedTonight = memory.disturbed;
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

        // Before the needs below rather than after them.
        // A tick that runs the happiness out has to end silent.
        // lose() clears the bubble, and a later line would sit on a grave.
        speak();

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

            // Said before the cost is paid rather than after it.
            // A tap that runs the happiness out leaves nothing to say.
            // Which is lose()'s doing, and it takes the bubble away.
            say(Saying::LetMeSleep);
            lose(config.disturbCost);
            return;
        }

        // Food a companion does not want is left uneaten.
        // Being offered it anyway is the third violation.
        // It costs less than waking one does.
        // A bowl pushed at a full animal beats a night broken in half.
        // It costs more than nothing, which is what it used to cost.
        // A tap nobody paid for left tapping all day a free action.
        if (!hungry())
        {
            ++pesterCount;
            say(Saying::NotHungry);
            lose(config.pesterCost);
            return;
        }

        hungerLevel = hungerLevel > config.feedRelief
                          ? hungerLevel - config.feedRelief
                          : 0;
        ++mealCount;
        say(Saying::Yum);
        gain(config.feedJoy);
    }

    void Pet::revive()
    {
        // What a new companion is, is what the constructor already says.
        // Written out again it would be a second list of one thing.
        // A field in one and not the other is the last one's hunger.
        // Nothing here holds a reference.
        // So assignment is the whole of it.
        *this = Pet(config);
    }

    PetMemory Pet::remember() const
    {
        return PetMemory{
            .ticks = elapsed,
            .state = petState,
            .saying = said,
            .sayingTicksLeft = sayingLeft,
            .hunger = hungerLevel,
            .happiness = happinessLevel,
            .meals = mealCount,
            .disturbances = disturbanceCount,
            .pesters = pesterCount,
            .disturbed = disturbedTonight};
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

    Saying Pet::saying() const noexcept
    {
        return said;
    }

    Tick Pet::sayingTicksLeft() const noexcept
    {
        return sayingLeft;
    }

    std::uint32_t Pet::meals() const noexcept
    {
        return mealCount;
    }

    std::uint32_t Pet::disturbances() const noexcept
    {
        return disturbanceCount;
    }

    std::uint32_t Pet::pesters() const noexcept
    {
        return pesterCount;
    }

    const PetConfig &Pet::settings() const noexcept
    {
        return config;
    }

    Tick Pet::cycleTicks() const noexcept
    {
        return config.dayTicks + config.nightTicks;
    }

    void Pet::say(const Saying line) noexcept
    {
        said = line;
        sayingLeft = config.sayingTicks;
    }

    void Pet::speak() noexcept
    {
        // A line already up runs its course before another may start.
        // So a need cannot cut an answer off mid-word.
        // And the bubble never flickers between two consecutive ticks.
        if (sayingLeft > 0)
        {
            --sayingLeft;

            if (sayingLeft == 0)
            {
                said = Saying::None;
            }

            return;
        }

        if (elapsed % config.chatterPeriodTicks != 0)
        {
            return;
        }

        if (petState == PetState::Asleep)
        {
            say(Saying::Zzz);
            return;
        }

        // A need is worth saying and idle chatter is only worth having.
        // So being hungry is what it talks about while it is hungry.
        if (hungry())
        {
            say(Saying::FeedMe);
            return;
        }

        say(kIdleSayings[idleIndex(elapsed)]);
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

        // Perishing is where every violation meets.
        // Neglect, disturbance and pestering spend the same number.
        // So any of the three can be what runs it out.
        if (happinessLevel == 0)
        {
            petState = PetState::Perished;

            // Nothing about a perished companion changes again.
            // A bubble left over one would be the last thing it did.
            said = Saying::None;
            sayingLeft = 0;
        }
    }

} // namespace antwika::companion
