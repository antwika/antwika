#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

#include <antwika/ecs/World.hpp>
#include <antwika/gfx/Point.hpp>

#include "antwika/game/BuildGhost.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/HousingLevel.hpp"
#include "antwika/game/MapView.hpp"
#include "antwika/game/OverlayField.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/Resource.hpp"
#include "antwika/game/RoadPlan.hpp"
#include "antwika/game/Ruin.hpp"
#include "antwika/game/Service.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    using antwika::ecs::World;
    using antwika::gfx::Point;

    /**
     * @brief One walker, as state that outlives a frame needs to know it.
     *
     * What a summary reports and a save file holds, so it carries where a
     * walker is and nothing about how it is being shown getting there.
     * WalkerSprite is the picture's answer to the same walker.
     */
    struct WalkerView
    {
        Cell at;
        Direction facing = Direction::East;

        /**
         * @brief Compare two walker views.
         * @param other The view to compare against.
         * @return True when both the cell and the facing match.
         */
        [[nodiscard]] bool operator==(const WalkerView &other) const = default;
    };

    /**
     * @brief One walker, as a frame needs to know it.
     *
     * Separate from WalkerView because a walker part of the way between
     * two cells is a fact about the picture and not about the state.
     * GameSummary and SaveGame both hold WalkerViews, so folding the two
     * render-side fields into that type would put them in a persisted
     * schema and in the value a replay determinism test compares -- which
     * is the same reason which of sixteen tiles a road shows is worked
     * out in GridScene and kept out of the snapshot entirely.
     */
    struct WalkerSprite
    {
        Cell at;
        Direction facing = Direction::East;

        /** @brief The cell being stepped out of, if there is one. */
        std::optional<Cell> from{};

        /**
         * @brief How many whole ticks of this step have gone.
         *
         * Counted up rather than down, since that is the direction a
         * fraction of the way there runs; Walker counts the same span
         * the other way because what it needs to know is when to move.
         */
        std::uint8_t ticksIntoStep = 0;

        /**
         * @brief What it hands out, or what risk it relieves.
         *
         * Here so a frame can draw what a walker is carrying without
         * being told a second time which resource that is: the kind is
         * the one fact carriedResource() answers from.
         */
        WalkerKind kind = WalkerKind::WaterCarrier;

        /** @brief How much of its resource is left to hand out. */
        std::int32_t carried = 0;

        /**
         * @brief Compare two walker sprites.
         * @param other The sprite to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(
            const WalkerSprite &other) const = default;
    };

    /**
     * @brief One building, as state that outlives a frame needs it.
     *
     * WalkerView's counterpart, and it is separate from BuildingSprite
     * for exactly WalkerView's reason: GameSummary holds these, and a
     * run and its replay are compared on that value.
     * What a building is holding right now belongs in the picture rather
     * than in the comparison, so it lives on the sprite.
     */
    struct BuildingView
    {
        /** @brief The minimum-x, minimum-y cell of its block. */
        Cell at;
        BuildingKind kind = BuildingKind::House;

        /**
         * @brief How much longer each service still reaches it.
         *
         * **On the view rather than only on the sprite, which is the
         * opposite call from stock, and deliberately.** What a building
         * is holding is a number a bar is drawn from; what is still
         * reaching it decides whether it gains risk, whether it may
         * grow, and how the city is rated -- so a run and its replay
         * disagreeing about it is a divergence, and GameSummary is
         * where a divergence is caught.
         *
         * Indexed by serviceIndex(), exactly as Coverage::ticksLeft is.
         * All zero for a building nothing has ever reached, which is
         * what an absent Coverage component means.
         */
        std::array<std::int32_t, kServiceCount> coverage{};

        /**
         * @brief How well the household living here lives.
         *
         * **On the view rather than only on the sprite, for the reason
         * coverage is.** A level is what the whole city is arranged to
         * raise, and it is a pure function of coverage, stock and
         * desirability -- all of which a replay regenerates -- so a run
         * and its replay disagreeing about it is a divergence, and
         * GameSummary is where a divergence is caught.
         *
         * HousingLevel::Tent for a building with no household, which
         * includes every kind nobody lives in.
         */
        HousingLevel level = HousingLevel::Tent;

        /**
         * @brief How many people live here right now.
         *
         * **On the view rather than only on the sprite, for the reason
         * the level is -- and it makes the replay comparison strictly
         * stronger.** Before this, occupancy reached GameSummary only as
         * CityRatings::population, a sum over the whole city, and a sum
         * hides a swap: two houses trading one occupant total the same,
         * so a live run and its replay could disagree about where the
         * city's people were and still compare equal. Compared per house
         * they cannot.
         *
         * Zero for a building with no household, which includes every
         * kind nobody lives in.
         */
        std::int32_t population = 0;

        /**
         * @brief How many of its people hold a job.
         *
         * One field for both readings, since a building is only
         * ever one of the two: a house counts its people with
         * jobs, off its Employment ledger, and a workplace
         * counts the people working there, off its Staff -- so
         * "Unemployed 4/16" and "Staff 6/8" both hang off it.
         */
        std::int32_t employed = 0;

        /**
         * @brief Compare two building views.
         * @param other The view to compare against.
         * @return True when the cell, the kind, the coverage, the level
         * and the occupancy match.
         */
        [[nodiscard]] bool operator==(
            const BuildingView &other) const = default;
    };

    /**
     * @brief One building, as a frame needs to know it.
     *
     * Carries what BuildingView does plus the stock a bar is drawn from,
     * which is the whole reason the two are separate types.
     */
    struct BuildingSprite
    {
        /** @brief The minimum-x, minimum-y cell of its block. */
        Cell at;
        BuildingKind kind = BuildingKind::House;

        /**
         * @brief How much of each resource it is holding.
         *
         * Indexed by resourceIndex(), exactly as Building::stock is, so
         * the picture and the component address one table one way.
         */
        std::array<std::int32_t, kResourceCount> stock{};

        /**
         * @brief How much longer each service still reaches it.
         *
         * A copy of the view's, so the hover panel can say which
         * services are keeping a building standing without reaching
         * back into the World for it -- the same reason stock is here.
         */
        std::array<std::int32_t, kServiceCount> coverage{};

        /**
         * @brief How well the household living here lives.
         *
         * A copy of the view's, so the hover panel can name the tier
         * without reaching back into the World for it -- the same reason
         * stock and coverage are here.
         * Once there is art per tier this is also what GridScene picks a
         * slot by; until then a house of any level draws the house slot,
         * and the level shows only in the panel.
         */
        HousingLevel level = HousingLevel::Tent;

        /**
         * @brief How many people live here right now.
         *
         * A copy of the view's, so the hover panel can say how full a
         * house is without reaching back into the World for it -- the
         * same reason stock, coverage and the level are here.
         * It is read against populationCapacityOf(level), which is why
         * the two travel together.
         */
        std::int32_t population = 0;

        /**
         * @brief How many of its people hold a job.
         *
         * One field for both readings, since a building is only
         * ever one of the two: a house counts its people with
         * jobs, off its Employment ledger, and a workplace
         * counts the people working there, off its Staff -- so
         * "Unemployed 4/16" and "Staff 6/8" both hang off it.
         */
        std::int32_t employed = 0;

        /**
         * @brief How close it is to catching fire, out of kMaxRisk.
         *
         * **On the sprite and not the view, which is stock's call and
         * for stock's reason**: the hover panel's risk section is
         * drawn from it, and a divergence in a risk surfaces as a
         * divergence in what burnt within a tick of mattering -- the
         * same argument that keeps every countdown out of the view.
         */
        std::int32_t fireRisk = 0;

        /** @brief How close it is to falling down, on the same terms. */
        std::int32_t collapseRisk = 0;

        /**
         * @brief Compare two building sprites.
         * @param other The sprite to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(
            const BuildingSprite &other) const = default;
    };

    /**
     * @brief One ruin, as the state and the picture both need it.
     *
     * **One type where a building has two**, and deliberately: the
     * view/sprite split exists so numbers that only exist to be drawn
     * -- stock, a gauge -- stay out of what GameSummary compares, and
     * a ruin has no such numbers. Where it is, what stood there and
     * whether it still burns are all facts a live run and its replay
     * must agree on, so the one type serves the snapshot, the summary
     * and the hover readout alike.
     *
     * The burn countdown is deliberately absent, as every Building
     * countdown is absent from BuildingView: a divergence there
     * surfaces as a divergence in the state within a tick of
     * mattering.
     */
    struct RuinView
    {
        /** @brief The minimum-x, minimum-y cell of its block. */
        Cell at;

        /** @brief What stood there; names the block and the sheet. */
        BuildingKind kind = BuildingKind::House;

        /** @brief Whether it is still alight. */
        RuinState state = RuinState::Burning;

        /**
         * @brief Compare two ruin views.
         * @param other The view to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(const RuinView &other) const
            = default;
    };

    /**
     * @brief What the pointer is over, and what to say about it.
     *
     * **A picture, and nothing but a picture**, on exactly BuildGhost's
     * terms: it is worked out from input::PointerHintChannel, which a
     * replay does not reproduce, so nothing may be folded from it into
     * anything a replay does reproduce and no sink may ever see one.
     *
     * It holds the sprite it found rather than a copy of that sprite's
     * numbers, so the panel a reader is shown and the bars drawn over
     * the thing itself cannot disagree -- they are one value read twice.
     *
     * At most one of the two is ever set, since hoverFor() answers with
     * the first thing it finds under the pointer.
     * Neither being set is the ordinary state of a pointer over bare
     * ground, and draws nothing.
     *
     * It is defined here rather than beside hoverFor(), unlike
     * BuildGhost, because it is made of the sprites above it and a
     * snapshot holds one.
     */
    struct HoverReadout
    {
        /** @brief The pixel the panel is pinned to. */
        Point anchor{};

        /** @brief The building under the pointer, if one is. */
        std::optional<BuildingSprite> building{};

        /** @brief The walker under the pointer, if one is. */
        std::optional<WalkerSprite> walker{};

        /** @brief The fire or debris under the pointer, if one is. */
        std::optional<RuinView> ruin{};

        /**
         * @brief Compare two readouts.
         * @param other The readout to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(
            const HoverReadout &other) const = default;
    };

    /**
     * @brief Everything one frame needs, and nothing that can change under
     * it.
     *
     * A plain value taken from the World rather than a reference into it,
     * for the reason apps/poker's TableSnapshot exists: it lets GridScene
     * be a pure function of its argument, so a picture can be asserted
     * call by call against a mock renderer instead of having to be looked
     * at.
     */
    struct SceneSnapshot
    {
        Camera camera;
        GridExtent extent;
        std::vector<Cell> paths;
        std::vector<WalkerSprite> walkers;
        std::vector<BuildingSprite> buildings;

        /**
         * @brief Every fire and every heap of debris, in painter's
         * order.
         *
         * Sorted on the very key the buildings are, since a ruin is a
         * block exactly as a building is and GridScene paints the two
         * lists through one merge -- see drawTerrain().
         */
        std::vector<RuinView> ruins;

        /**
         * @brief Whether the simulation was held still when this was
         * taken.
         *
         * Simulation state read into the picture, exactly as the camera
         * is: PauseState owns it, a snapshot copies it, and nothing here
         * writes back to it.
         *
         * It is here because a walker part of the way between two cells
         * is drawn from two clocks -- the whole ticks of its step, which
         * a pause stops, and how far through the current tick a frame
         * falls, which a pause does not. Left to itself the second one
         * keeps running, so a frozen walker slides forward through every
         * tick and snaps back at the start of the next one, for as long
         * as the run is paused.
         * A scene told the run is held draws it at its step's own phase
         * and no further -- see GridScene::draw().
         */
        bool paused = false;


        /**
         * @brief The run of road a drag under way would lay.
         *
         * **Unlike the ghost and the readout below it, this one is
         * simulation state**: it is worked out from RoadDrag, which
         * GridSink writes inside the tick path from the recorded press
         * and movements, so a replay arrives at the same route. That is
         * why it is filled in once a tick rather than once a frame.
         *
         * Empty when no drag is under way, which draws nothing.
         */
        RoadPlan plan;

        /**
         * @brief Where the selected tool would land if it were clicked.
         *
         * The one member snapshotOf() does not fill in: it is a picture
         * worked out on the render side from a channel no replay
         * reproduces, so nothing about it may be taken from the World --
         * see BuildGhost. Invisible by default, so a snapshot nobody has
         * given one draws none.
         */
        BuildGhost ghost;

        /**
         * @brief What the pointer is over, and what to say about it.
         *
         * The second member snapshotOf() does not fill in, and for the
         * ghost's reason exactly: it comes off a channel no replay
         * reproduces, so nothing about it may be taken from the World.
         * Empty by default, so a snapshot nobody has given one says
         * nothing.
         */
        HoverReadout hover;
        /**
         * @brief Which picture of the city this is.
         *
         * Normal is the city itself and every other value is one number
         * about it painted over the ground -- see MapView.
         * Carried on the snapshot rather than read off MapViewState by
         * whoever draws, for the reason the pause is: a snapshot draws
         * the same picture wherever it is drawn, and the frames between
         * two ticks must not see a view the tick did not.
         */
        MapView view = MapView::Normal;

        /**
         * @brief How strongly that view paints each cell.
         *
         * Empty for the normal view, which is what makes it the one
         * with nothing painted over it rather than the one with a
         * branch of its own.
         * Filled once a tick rather than once a frame, like the plan
         * beside it: it is derived from state a replay reproduces, so
         * there is nothing about it a frame could see that the tick did
         * not.
         */
        OverlayField overlay;

        /**
         * @brief Compare two snapshots.
         * @param other The snapshot to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(
            const SceneSnapshot &other) const = default;
    };

    /**
     * @brief Take a snapshot of what to draw.
     *
     * Paths come from the index rather than from a view over the world,
     * because the index is already ordered and a view is not ordered by
     * anything a reader can name.
     *
     * @param world Read for the walkers, as of its last commit().
     * @param paths Read for the path cells.
     * @param camera The camera to draw through.
     * @param extent The bounds to draw within.
     * @param paused Whether the run is being held still right now, off
     * PauseState. Defaulted to the state a run begins in, so a caller
     * wanting the walkers rather than the picture -- a summary, a save
     * -- need not answer a question it has no stake in.
     * @return The frame's description, with no ghost; whoever draws
     * fills that in from ghostFor().
     */
    [[nodiscard]] SceneSnapshot snapshotOf(
        const World &world,
        const PathIndex &paths,
        const Camera &camera,
        GridExtent extent,
        bool paused = false);

    /**
     * @brief List every walker as state rather than as a picture.
     *
     * What a summary and a save file want, so neither has to take a whole
     * frame's worth of camera, paths and buildings to get at the walkers,
     * and neither picks up the two fields that only exist to draw with.
     *
     * @param world Read for the walkers, as of its last commit().
     * @return One view per walker, in the world's own order.
     */
    [[nodiscard]] std::vector<WalkerView> walkerViewsOf(const World &world);

    /**
     * @brief List every building as state rather than as a picture.
     *
     * walkerViewsOf()'s counterpart, and it exists for the same reason:
     * a summary wants where each building is and what kind it is, and
     * must not pick up the stock that only exists to draw a bar from.
     *
     * The world's own order, which is the order the buildings were put
     * up in -- a summary reports a session rather than a screen, so the
     * back-to-front order a frame needs would say nothing here.
     *
     * @param world Read for the buildings, as of its last commit().
     * @return One view per building, in the world's own order.
     */
    [[nodiscard]] std::vector<BuildingView> buildingViewsOf(
        const World &world);

    /**
     * @brief List every ruin, in the world's own order.
     *
     * buildingViewsOf()'s counterpart for what has burnt down: a
     * summary and a save both want the fires and the debris, since a
     * run and its replay disagreeing about either is a divergence.
     * The world's own order for the reason buildingViewsOf() keeps
     * it; snapshotOf() is what sorts a copy into painter's order.
     *
     * @param world Read for the ruins, as of its last commit().
     * @return One view per ruin, in the world's own order.
     */
    [[nodiscard]] std::vector<RuinView> ruinViewsOf(const World &world);

} // namespace antwika::game
