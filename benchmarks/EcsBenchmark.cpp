#include <chrono>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <string_view>
#include <utility>
#include <vector>

#include <antwika/ecs/OpenPhase.hpp>
#include <antwika/ecs/View.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/event/EventDispatcher.hpp>
#include <antwika/event/IEventSink.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickedEventDispatcher.hpp>
#include <antwika/time/Tick.hpp>

namespace
{

    using antwika::ecs::Entity;
    using antwika::ecs::OpenPhase;
    using antwika::ecs::World;
    using antwika::log::ILogger;
    using antwika::log::Level;

    using Clock = std::chrono::steady_clock;

    class SilentLogger final : public ILogger
    {
    public:
        void log(Level, std::string_view) noexcept override
        {
        }
    };

    struct Cell final
    {
        bool alive{};
    };

    struct Position final
    {
        std::int32_t x{};
        std::int32_t y{};
    };

    struct Velocity final
    {
        std::int32_t dx{};
        std::int32_t dy{};
    };

    struct Health final
    {
        std::int32_t value{};
    };

    struct Label final
    {
        std::uint32_t id{};
    };

    struct Result final
    {
        double millis{};
        std::uint64_t checksum{};
    };

    [[nodiscard]] double millisSince(Clock::time_point start)
    {
        const auto elapsed = Clock::now() - start;
        return std::chrono::duration<double, std::milli>(elapsed).count();
    }

    // Mirrors what SystemScheduler::run does in the game app: eleven
    // phases per tick, each ending in a World::commit, over a world
    // where almost nothing changed.
    [[nodiscard]] Result benchmarkCommit()
    {
        constexpr std::size_t kEntities = 20000;
        constexpr int kPhases = 11;
        constexpr int kTicks = 100;

        SilentLogger logger;
        World world(logger);

        std::vector<Entity> entities;
        entities.reserve(kEntities);

        {
            const OpenPhase phase(world);

            for (std::size_t at = 0; at < kEntities; ++at)
            {
                const auto entity = world.create();
                const auto value = static_cast<std::int32_t>(at);
                world.add<Position>(entity, Position{value, value});
                world.add<Velocity>(entity, Velocity{1, 1});
                world.add<Health>(entity, Health{value});
                world.add<Label>(
                    entity, Label{static_cast<std::uint32_t>(at)});
                entities.push_back(entity);
            }
        }

        std::uint64_t checksum = 0;
        const auto start = Clock::now();

        for (int tick = 0; tick < kTicks; ++tick)
        {
            for (int phase = 0; phase < kPhases; ++phase)
            {
                const OpenPhase openPhase(world);
                const auto at =
                    static_cast<std::size_t>(tick * kPhases + phase)
                    % kEntities;
                world.set<Health>(entities[at], Health{tick});
            }

            checksum += static_cast<std::uint64_t>(
                world.get<Health>(entities[0]).value);
        }

        return Result{millisSince(start), checksum};
    }

    // Mirrors LifeSystem: nine component reads per cell per tick, which
    // is the pattern that pays for every World::get lookup.
    [[nodiscard]] Result benchmarkGridReads()
    {
        constexpr std::int32_t kSide = 200;
        constexpr int kTicks = 20;

        SilentLogger logger;
        World world(logger);

        std::vector<Entity> grid;
        grid.reserve(static_cast<std::size_t>(kSide) * kSide);

        {
            const OpenPhase phase(world);

            for (std::int32_t y = 0; y < kSide; ++y)
            {
                for (std::int32_t x = 0; x < kSide; ++x)
                {
                    const auto entity = world.create();
                    world.add<Cell>(entity, Cell{(x + y) % 3 == 0});
                    grid.push_back(entity);
                }
            }
        }

        const auto at = [&grid](std::int32_t x, std::int32_t y)
        {
            return grid[static_cast<std::size_t>(y) * kSide + x];
        };

        std::uint64_t checksum = 0;
        const auto start = Clock::now();

        for (int tick = 0; tick < kTicks; ++tick)
        {
            const OpenPhase phase(world);

            for (std::int32_t y = 1; y < kSide - 1; ++y)
            {
                for (std::int32_t x = 1; x < kSide - 1; ++x)
                {
                    int alive = 0;

                    for (std::int32_t dy = -1; dy <= 1; ++dy)
                    {
                        for (std::int32_t dx = -1; dx <= 1; ++dx)
                        {
                            if (dx == 0 && dy == 0)
                            {
                                continue;
                            }

                            if (world.get<Cell>(at(x + dx, y + dy)).alive)
                            {
                                ++alive;
                            }
                        }
                    }

                    const auto was = world.get<Cell>(at(x, y)).alive;
                    const auto now =
                        was ? (alive == 2 || alive == 3) : (alive == 3);
                    world.set<Cell>(at(x, y), Cell{now});

                    checksum += now ? 1U : 0U;
                }
            }
        }

        return Result{millisSince(start), checksum};
    }

    // Mirrors the roughly forty world.view<A, B>() sites in the game
    // app, where a view is built afresh every tick.
    [[nodiscard]] Result benchmarkViews()
    {
        constexpr std::size_t kEntities = 20000;
        constexpr int kRounds = 500;

        SilentLogger logger;
        World world(logger);

        {
            const OpenPhase phase(world);

            for (std::size_t at = 0; at < kEntities; ++at)
            {
                const auto entity = world.create();
                const auto value = static_cast<std::int32_t>(at);
                world.add<Position>(entity, Position{value, value});

                if (at % 2 == 0)
                {
                    world.add<Velocity>(entity, Velocity{1, 1});
                }
            }
        }

        std::uint64_t checksum = 0;
        const auto start = Clock::now();

        for (int round = 0; round < kRounds; ++round)
        {
            for (const auto entity : world.view<Position, Velocity>())
            {
                checksum += static_cast<std::uint64_t>(
                    world.get<Position>(entity).x);
            }
        }

        return Result{millisSince(start), checksum};
    }

    // Mass despawn, which is where a linear erase per removed component
    // turns quadratic.
    [[nodiscard]] Result benchmarkTeardown()
    {
        constexpr std::size_t kEntities = 5000;
        constexpr int kRounds = 20;

        SilentLogger logger;
        World world(logger);

        std::uint64_t checksum = 0;
        const auto start = Clock::now();

        for (int round = 0; round < kRounds; ++round)
        {
            std::vector<Entity> entities;
            entities.reserve(kEntities);

            {
                const OpenPhase spawningPhase(world);

                for (std::size_t at = 0; at < kEntities; ++at)
                {
                    const auto entity = world.create();
                    const auto value = static_cast<std::int32_t>(at);
                    world.add<Position>(entity, Position{value, value});
                    world.add<Velocity>(entity, Velocity{1, 1});
                    world.add<Health>(entity, Health{value});
                    world.add<Label>(
                        entity, Label{static_cast<std::uint32_t>(at)});
                    entities.push_back(entity);
                }
            }

            {
                const OpenPhase clearingPhase(world);

                for (const auto entity : entities)
                {
                    world.destroy(entity);
                }
            }

            checksum += entities.size();
        }

        return Result{millisSince(start), checksum};
    }

    class CountingSink final : public antwika::event::IEventSink
    {
    public:
        void handle(const antwika::event::Event &event) override
        {
            if (event.name == antwika::engine::events::kTick)
            {
                ++seen;
            }
        }

        [[nodiscard]] std::uint64_t count() const noexcept
        {
            return seen;
        }

    private:
        std::uint64_t seen{};
    };

    class CountingTickSink final : public antwika::event::ITickEventSink
    {
    public:
        void handle(const antwika::event::TickEvent &event) override
        {
            if (event.event.name == antwika::engine::events::kTick)
            {
                ++seen;
            }
        }

        [[nodiscard]] std::uint64_t count() const noexcept
        {
            return seen;
        }

    private:
        std::uint64_t seen{};
    };

    // Mirrors an engine tick fanning out to every sink an app registers,
    // where each sink compares the event name against a constant.
    [[nodiscard]] Result benchmarkEvents(const char *payload)
    {
        constexpr std::size_t kSinks = 20;
        constexpr int kEvents = 200000;

        std::vector<CountingSink> sinks(kSinks);
        std::vector<std::reference_wrapper<antwika::event::IEventSink>> refs;
        for (auto &sink : sinks)
        {
            refs.emplace_back(sink);
        }

        CountingTickSink recorder;
        std::vector<std::reference_wrapper<antwika::event::ITickEventSink>>
            timed;
        timed.emplace_back(recorder);

        antwika::event::EventDispatcher inner(std::move(refs));
        antwika::event::TickedEventDispatcher dispatcher(
            inner, std::move(timed));

        const auto start = Clock::now();

        for (int at = 0; at < kEvents; ++at)
        {
            dispatcher.setTick(static_cast<antwika::time::Tick>(at));
            dispatcher.dispatch(antwika::event::Event{
                .name = antwika::engine::events::kTick, .payload = payload});
        }

        std::uint64_t checksum = recorder.count();
        for (const auto &sink : sinks)
        {
            checksum += sink.count();
        }

        return Result{millisSince(start), checksum};
    }

    void report(const char *name, const Result &result)
    {
        std::printf(
            "%-16s %10.2f ms   (checksum %llu)\n",
            name,
            result.millis,
            static_cast<unsigned long long>(result.checksum));
    }

}

int main()
{
    std::printf("%-16s %13s\n", "scenario", "elapsed");
    report("commit", benchmarkCommit());
    report("grid-reads", benchmarkGridReads());
    report("views", benchmarkViews());
    report("teardown", benchmarkTeardown());
    report("events-sso", benchmarkEvents("ok"));
    report(
        "events-heap",
        benchmarkEvents(
            "a payload comfortably past the small string buffer limit"));
    return 0;
}
