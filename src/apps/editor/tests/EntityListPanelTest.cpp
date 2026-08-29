#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <antwika/ecs/Entity.hpp>
#include <antwika/gfx/NullBackend.hpp>
#include <antwika/input/NullInputBackend.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/map/MapFile.hpp>
#include <antwika/map/Marker.hpp>
#include <antwika/ui/DrawCommand.hpp>
#include <antwika/ui/Interactions.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/widget/WidgetId.hpp>

#include "antwika/editor/Editor.hpp"
#include "antwika/editor/fakes/EditorProbe.hpp"
#include "antwika/editor/ui/WidgetIds.hpp"

using antwika::editor::EntityKind;
using antwika::editor::EntityRow;
using antwika::editor::Tool;
using antwika::editor::getEntityRowWidget;
using antwika::editor::kMaxEntityRows;
using antwika::log::mocks::MockLogger;
using antwika::map::Marker;
using antwika::voxel::VoxelPosition;
using ::testing::NiceMock;

namespace
{

    constexpr std::string_view kMissingMapPath =
        "assets/maps/no-such-map.json";

    constexpr VoxelPosition kFoodPosition{.x = 20, .y = 8, .z = 20};

    constexpr VoxelPosition kWaterPosition{.x = 24, .y = 8, .z = 24};

    constexpr VoxelPosition kStartPosition{.x = 12, .y = 8, .z = 12};

    constexpr VoxelPosition kExitPosition{.x = 16, .y = 8, .z = 16};

    constexpr VoxelPosition kCheckpointPosition{.x = 28, .y = 8, .z = 28};

    struct PanelEdgeCase final
    {
        antwika::widget::WidgetId widget;

        std::uint32_t antwika::editor::PanelSizes::*extent;
    };

    constexpr std::array<PanelEdgeCase, 7> kEveryPanelEdge{
        PanelEdgeCase{
            antwika::editor::kToolPanelEdgeWidget,
            &antwika::editor::PanelSizes::toolWidth},
        PanelEdgeCase{
            antwika::editor::kEntityListEdgeWidget,
            &antwika::editor::PanelSizes::entityWidth},
        PanelEdgeCase{
            antwika::editor::kDrawColumnEdgeWidget,
            &antwika::editor::PanelSizes::inspectWidth},
        PanelEdgeCase{
            antwika::editor::kRailEdgeWidget,
            &antwika::editor::PanelSizes::railWidth},
        PanelEdgeCase{
            antwika::editor::kPlanFirstEdgeWidget,
            &antwika::editor::PanelSizes::planFirstWidth},
        PanelEdgeCase{
            antwika::editor::kPlanSecondEdgeWidget,
            &antwika::editor::PanelSizes::planSecondWidth},
        PanelEdgeCase{
            antwika::editor::kPlanDetailEdgeWidget,
            &antwika::editor::PanelSizes::cardWidth}};

    class EntityListPanelTest : public ::testing::Test
    {
    protected:
        EntityListPanelTest()
        {
            probe.preferences().tool = Tool::Select;
        }

        [[nodiscard]] std::size_t placeOfKind(const EntityKind kind)
        {
            const auto &rows = probe.entityList().rows;

            return static_cast<std::size_t>(
                std::distance(
                    rows.begin(),
                    std::ranges::find(rows, kind, &EntityRow::kind)));
        }

        [[nodiscard]] std::size_t placeOfNoKind()
        {
            const auto &rows = probe.entityList().rows;

            return static_cast<std::size_t>(
                std::distance(
                    rows.begin(),
                    std::ranges::find_if(
                        rows,
                        [](const EntityRow &row)
                        { return !row.kind.has_value(); })));
        }

        [[nodiscard]] std::size_t placeOfCharacter(
            const std::size_t characterIndex)
        {
            const auto &rows = probe.entityList().rows;

            return static_cast<std::size_t>(
                std::distance(
                    rows.begin(),
                    std::ranges::find_if(
                        rows,
                        [characterIndex](const EntityRow &row)
                        {
                            return row.kind == EntityKind::Character
                                   && row.characterIndex
                                          == characterIndex;
                        })));
        }

        void layItems()
        {
            probe.document.map.markers.positionsOf(Marker::Food)
                .push_back(kFoodPosition);
            probe.document.map.markers.positionsOf(Marker::Water)
                .push_back(kWaterPosition);
            probe.spawnCharacters();
        }

        NiceMock<MockLogger> logger;
        antwika::gfx::NullBackend backend{logger};
        antwika::input::NullInputBackend inputs{logger};
        antwika::editor::Editor editor{
            logger, backend, inputs, std::string(kMissingMapPath)};
        antwika::editor::fakes::EditorProbe probe{editor};
    };

}

TEST_F(EntityListPanelTest, EntityRows_NameTheCharactersTheDocumentHolds)
{
    probe.document.map.characters.at(0).name = "Ari";
    probe.spawnCharacters();
    probe.pumpFrame();

    const auto &row = probe.entityList().rows.at(placeOfCharacter(0));

    EXPECT_EQ(row.kind, EntityKind::Character);
    EXPECT_EQ(row.name, "Ari");
    EXPECT_TRUE(row.player);
}

TEST_F(EntityListPanelTest, EntityRows_CallANamelessCharacterUnnamed)
{
    probe.document.map.characters.push_back(
        antwika::map::Character{
            .components =
                std::vector<std::string>{"component::CharacterIndex"}});
    probe.spawnCharacters();
    probe.pumpFrame();

    const auto &row = probe.entityList().rows.at(placeOfCharacter(1));

    EXPECT_EQ(row.kind, EntityKind::Character);
    EXPECT_EQ(row.name, "unnamed");
    EXPECT_FALSE(row.player);
    EXPECT_FALSE(row.aimPosition.has_value());
}

TEST_F(EntityListPanelTest, EntityRows_ShowACharacterTheDocumentHasDropped)
{
    probe.document.map.characters.push_back(
        antwika::map::Character{
            .name = "Gone",
            .components =
                std::vector<std::string>{"component::CharacterIndex"}});
    probe.spawnCharacters();
    probe.document.map.characters.pop_back();
    probe.pumpFrame();

    const auto &row = probe.entityList().rows.at(placeOfCharacter(1));

    EXPECT_EQ(row.kind, EntityKind::Character);
    EXPECT_EQ(row.name, "unnamed");
    EXPECT_EQ(row.cellPosition, VoxelPosition{});
}

TEST_F(EntityListPanelTest, EntityRows_ListTheEntityNoComponentNames)
{
    probe.pumpFrame();

    const auto &rows = probe.entityList().rows;

    ASSERT_LT(placeOfNoKind(), rows.size());

    const auto &row = rows.at(placeOfNoKind());

    EXPECT_THAT(row.name, ::testing::StartsWith("Entity "));
    EXPECT_FALSE(row.aimPosition.has_value());
}

TEST_F(EntityListPanelTest, EntityRows_FollowTheOrderTheWorldHoldsThem)
{
    layItems();
    probe.pumpFrame();

    const auto &rows = probe.entityList().rows;

    ASSERT_GT(rows.size(), 3U);

    for (std::size_t place = 1; place < rows.size(); ++place)
    {
        if (rows.at(place - 1).entity == antwika::ecs::kNullEntity
            || rows.at(place).entity == antwika::ecs::kNullEntity)
        {
            continue;
        }

        EXPECT_LT(
            antwika::ecs::getRawValue(rows.at(place - 1).entity),
            antwika::ecs::getRawValue(rows.at(place).entity));
    }

    EXPECT_EQ(
        rows.at(placeOfKind(EntityKind::Water)).cellPosition, kWaterPosition);
    EXPECT_EQ(
        rows.at(placeOfKind(EntityKind::Food)).cellPosition, kFoodPosition);
}

TEST_F(EntityListPanelTest, EntityRows_ListTheStartTheExitAndEveryCheckpoint)
{
    probe.document.map.spawnCubePosition = kStartPosition;
    probe.document.map.exitCubePosition = kExitPosition;
    probe.document.map.markers.positionsOf(Marker::Checkpoint)
        .push_back(kCheckpointPosition);
    probe.pumpFrame();

    const auto &rows = probe.entityList().rows;

    ASSERT_LT(placeOfKind(EntityKind::Start), rows.size());
    ASSERT_LT(placeOfKind(EntityKind::Exit), rows.size());
    ASSERT_LT(placeOfKind(EntityKind::Checkpoint), rows.size());

    const auto &startRow = rows.at(placeOfKind(EntityKind::Start));
    const auto &exitRow = rows.at(placeOfKind(EntityKind::Exit));
    const auto &checkpointRow =
        rows.at(placeOfKind(EntityKind::Checkpoint));

    EXPECT_EQ(startRow.name, "Start");
    EXPECT_EQ(startRow.cellPosition, kStartPosition);
    EXPECT_TRUE(startRow.aimPosition.has_value());
    EXPECT_EQ(exitRow.name, "Exit");
    EXPECT_EQ(exitRow.cellPosition, kExitPosition);
    EXPECT_EQ(checkpointRow.name, "Checkpoint");
    EXPECT_EQ(checkpointRow.cellPosition, kCheckpointPosition);
}

TEST_F(EntityListPanelTest, EntityRows_ListNoStartUntilTheMapHoldsOne)
{
    probe.pumpFrame();

    const auto &rows = probe.entityList().rows;

    EXPECT_EQ(placeOfKind(EntityKind::Start), rows.size());
    EXPECT_EQ(placeOfKind(EntityKind::Exit), rows.size());
    EXPECT_EQ(placeOfKind(EntityKind::Checkpoint), rows.size());
}

TEST_F(EntityListPanelTest, EntityRows_FollowACheckpointLaidAfterTheSpawn)
{
    probe.pumpFrame();

    EXPECT_EQ(
        placeOfKind(EntityKind::Checkpoint),
        probe.entityList().rows.size());

    probe.document.map.markers.positionsOf(Marker::Checkpoint)
        .push_back(kCheckpointPosition);
    probe.pumpFrame();

    const auto &rows = probe.entityList().rows;

    ASSERT_LT(placeOfKind(EntityKind::Checkpoint), rows.size());
    EXPECT_EQ(
        rows.at(placeOfKind(EntityKind::Checkpoint)).cellPosition,
        kCheckpointPosition);
}

TEST_F(EntityListPanelTest, EntityRows_LetGoOfAnExitTakenOffTheMap)
{
    probe.document.map.exitCubePosition = kExitPosition;
    probe.pumpFrame();

    ASSERT_LT(placeOfKind(EntityKind::Exit), probe.entityList().rows.size());

    probe.document.map.exitCubePosition.reset();
    probe.pumpFrame();

    EXPECT_EQ(
        placeOfKind(EntityKind::Exit), probe.entityList().rows.size());
}

TEST_F(EntityListPanelTest, EntityRows_StandOnePadEntityPerCubeTheMapMarks)
{
    probe.document.map.spawnCubePosition = kStartPosition;
    probe.document.map.markers.positionsOf(Marker::Checkpoint)
        .push_back(kCheckpointPosition);
    probe.pumpFrame();
    probe.pumpFrame();

    const auto &rows = probe.entityList().rows;
    std::size_t startCount = 0;

    for (const auto &row : rows)
    {
        if (row.kind == EntityKind::Start)
        {
            ++startCount;
        }
    }

    EXPECT_EQ(startCount, 1U);
}

TEST_F(EntityListPanelTest, PressEntityRow_PicksTheCheckpointTheRowStandsFor)
{
    probe.document.map.markers.positionsOf(Marker::Checkpoint)
        .push_back(kCheckpointPosition);
    probe.pumpFrame();

    EXPECT_TRUE(probe.pressEntityRow(placeOfKind(EntityKind::Checkpoint)));
    EXPECT_EQ(probe.entityPick().kind, EntityKind::Checkpoint);
    EXPECT_EQ(probe.entityPick().position, kCheckpointPosition);
    EXPECT_TRUE(probe.isEntitySectionShown());
}

TEST_F(EntityListPanelTest, PressEntityRow_PicksTheStartTheRowStandsFor)
{
    probe.document.map.spawnCubePosition = kStartPosition;
    probe.pumpFrame();

    EXPECT_TRUE(probe.pressEntityRow(placeOfKind(EntityKind::Start)));
    EXPECT_EQ(probe.entityPick().kind, EntityKind::Start);
    EXPECT_EQ(probe.entityPick().position, kStartPosition);
    EXPECT_TRUE(probe.isEntitySectionShown());
}

TEST_F(EntityListPanelTest, PressEntityRow_PicksTheExitTheRowStandsFor)
{
    probe.document.map.exitCubePosition = kExitPosition;
    probe.pumpFrame();

    EXPECT_TRUE(probe.pressEntityRow(placeOfKind(EntityKind::Exit)));
    EXPECT_EQ(probe.entityPick().kind, EntityKind::Exit);
    EXPECT_EQ(probe.entityPick().position, kExitPosition);
    EXPECT_TRUE(probe.isEntitySectionShown());
}

TEST_F(EntityListPanelTest, EntityRows_StandTheirNamesAtTheLeftOfTheButton)
{
    layItems();

    const auto frame = probe.layoutUi();
    const auto place = placeOfKind(EntityKind::Food);
    const auto rowRect =
        frame.rects.getWidgetRect(getEntityRowWidget(place));

    ASSERT_TRUE(rowRect.has_value());
    ASSERT_LT(place, probe.entityList().rows.size());

    const auto &name = probe.entityList().rows.at(place).name;
    auto foundName = false;

    for (const auto &command : frame.drawList)
    {
        const auto *text = std::get_if<antwika::ui::DrawText>(&command);

        if (text == nullptr || text->text != name)
        {
            continue;
        }

        foundName = true;

        EXPECT_LT(
            text->originPoint.x - rowRect->originPoint.x,
            static_cast<std::int32_t>(rowRect->size.width) / 2);
    }

    EXPECT_TRUE(foundName);
}

TEST_F(EntityListPanelTest, BeginEdgeDrag_KeepsTheWidthEveryEdgeAsksFor)
{
    probe.pumpFrame();

    std::uint32_t askedExtent = 60;

    for (const auto &row : kEveryPanelEdge)
    {
        ++askedExtent;

        EXPECT_TRUE(
            probe.beginEdgeDrag(
                antwika::ui::Interactions{
                    .edge = antwika::ui::EdgeChange{
                        .edgeWidget = row.widget, .extent = askedExtent}}));
        EXPECT_EQ(probe.preferences().panelSizes.*row.extent, askedExtent);
        EXPECT_EQ(probe.pointer.heldEdgeWidget, row.widget);

        probe.endEdgeDrag();

        EXPECT_EQ(probe.pointer.heldEdgeWidget, antwika::widget::kNoWidget);
    }
}

TEST_F(EntityListPanelTest, BeginEdgeDrag_LeavesAnEdgeItDoesNotKnowAlone)
{
    probe.pumpFrame();

    EXPECT_FALSE(
        probe.beginEdgeDrag(
            antwika::ui::Interactions{
                .edge = antwika::ui::EdgeChange{
                    .edgeWidget = antwika::widget::WidgetId{99999},
                    .extent = 123}}));
    EXPECT_EQ(probe.pointer.heldEdgeWidget, antwika::widget::kNoWidget);
}

TEST_F(EntityListPanelTest, BeginEdgeDrag_LeavesAFrameWithNoEdgeAlone)
{
    probe.pumpFrame();

    EXPECT_FALSE(probe.beginEdgeDrag(antwika::ui::Interactions{}));
    EXPECT_EQ(probe.pointer.heldEdgeWidget, antwika::widget::kNoWidget);
}

TEST_F(EntityListPanelTest, PressEntityRow_PicksTheCharacterTheRowNames)
{
    probe.pumpFrame();
    probe.preferences().tool = Tool::Brush;

    EXPECT_TRUE(
        probe.consumeWidgets(
            antwika::ui::Interactions{
                .activatedWidget =
                    getEntityRowWidget(placeOfCharacter(0))}));
    EXPECT_EQ(probe.entityPick().kind, EntityKind::Character);
    EXPECT_EQ(probe.preferences().tool, Tool::Select);
    EXPECT_FALSE(probe.entityPick().dragging);
    EXPECT_TRUE(probe.isEntitySectionShown());
}

TEST_F(EntityListPanelTest, PressEntityRow_PicksTheItemTheRowNames)
{
    layItems();
    probe.pumpFrame();

    const auto place = placeOfKind(EntityKind::Food);

    EXPECT_TRUE(
        probe.consumeWidgets(
            antwika::ui::Interactions{
                .activatedWidget = getEntityRowWidget(place)}));
    EXPECT_EQ(probe.entityPick().kind, EntityKind::Food);
    EXPECT_EQ(probe.entityPick().position, kFoodPosition);
    EXPECT_TRUE(probe.isEntitySectionShown());
}

TEST_F(EntityListPanelTest, PressEntityRow_AimsTheCameraOnlyOnASecondPress)
{
    probe.pumpFrame();

    const auto opening = probe.cameraRig().view.transform;
    const auto place = placeOfCharacter(0);

    ASSERT_TRUE(probe.entityList().rows.at(place).aimPosition.has_value());

    const auto standing =
        *probe.entityList().rows.at(place).aimPosition;
    const antwika::ui::Interactions pressInteractions{
        .activatedWidget = getEntityRowWidget(place)};

    EXPECT_TRUE(probe.consumeWidgets(pressInteractions));
    EXPECT_EQ(probe.cameraRig().view.transform, opening);
    EXPECT_TRUE(probe.consumeWidgets(pressInteractions));
    EXPECT_EQ(probe.cameraRig().view.transform.position, standing);
    EXPECT_FLOAT_EQ(probe.cameraRig().view.transform.yaw, opening.yaw);
    EXPECT_FLOAT_EQ(probe.cameraRig().view.transform.pitch, opening.pitch);
}

TEST_F(EntityListPanelTest, PressEntityRow_LeavesTheCameraForTheEyeRow)
{
    probe.pumpFrame();

    const auto place = placeOfNoKind();

    ASSERT_LT(place, probe.entityList().rows.size());

    const auto opening = probe.cameraRig().view.transform;
    const antwika::ui::Interactions pressInteractions{
        .activatedWidget = getEntityRowWidget(place)};

    EXPECT_TRUE(probe.consumeWidgets(pressInteractions));
    EXPECT_TRUE(probe.consumeWidgets(pressInteractions));
    EXPECT_EQ(probe.cameraRig().view.transform, opening);
    EXPECT_FALSE(probe.entityPick().kind.has_value());
}

TEST_F(EntityListPanelTest, EntityRows_KeepThePickAcrossAFreshSpawn)
{
    probe.pumpFrame();

    ASSERT_TRUE(
        probe.consumeWidgets(
            antwika::ui::Interactions{
                .activatedWidget =
                    getEntityRowWidget(placeOfCharacter(0))}));

    const auto entityBefore =
        probe.entityList().rows.at(placeOfCharacter(0)).entity;
    const auto cell = probe.entityPick().position;

    probe.spawnCharacters();
    probe.pumpFrame();

    const auto &row = probe.entityList().rows.at(placeOfCharacter(0));

    EXPECT_NE(row.entity, entityBefore);
    EXPECT_EQ(row.cellPosition, cell);
    EXPECT_EQ(probe.entityPick().kind, EntityKind::Character);
    EXPECT_TRUE(probe.isEntitySectionShown());
}

TEST_F(EntityListPanelTest, EntityRowWidget_StaysInsideItsReservedBlock)
{
    probe.entityList().rows.assign(kMaxEntityRows + 8, EntityRow{});

    EXPECT_TRUE(
        probe.consumeWidgets(
            antwika::ui::Interactions{
                .activatedWidget = getEntityRowWidget(kMaxEntityRows - 1)}));
    EXPECT_FALSE(
        probe.consumeWidgets(
            antwika::ui::Interactions{
                .activatedWidget = getEntityRowWidget(kMaxEntityRows)}));
}

TEST_F(EntityListPanelTest, PressEntityRow_TakesNothingPastTheLastRow)
{
    probe.pumpFrame();

    EXPECT_FALSE(probe.pressEntityRow(probe.entityList().rows.size()));
}

TEST_F(EntityListPanelTest, CarryEntityListScroll_MovesTheListWhereItIsSent)
{
    antwika::ui::Interactions interactions{};

    interactions.scrollChange = antwika::ui::ScrollChange{
        .areaWidget = antwika::editor::kEntityListScrollWidget, .line = 7};

    probe.carryEntityListScroll(interactions);

    EXPECT_EQ(probe.entityList().scrollLine, 7U);

    interactions.scrollChange = antwika::ui::ScrollChange{
        .areaWidget = antwika::editor::kStatusBarWidget, .line = 3};

    probe.carryEntityListScroll(interactions);

    EXPECT_EQ(probe.entityList().scrollLine, 7U);
}

TEST_F(EntityListPanelTest, CarryEntityListScroll_HoldsTheTrackWhilePressed)
{
    antwika::ui::Interactions interactions{};

    interactions.activatedWidget = antwika::editor::kEntityListScrollWidget;
    interactions.areaPress = antwika::ui::TextAreaPress{
        .areaWidget = antwika::editor::kEntityListScrollWidget};

    probe.carryEntityListScroll(interactions);

    EXPECT_TRUE(probe.entityList().trackHeld);

    interactions.areaPress.reset();

    probe.carryEntityListScroll(interactions);

    EXPECT_FALSE(probe.entityList().trackHeld);
}

TEST_F(EntityListPanelTest, EntityListHover_KeepsTheWheelFromTheWorld)
{
    probe.pointer.hoveredWidget = antwika::widget::kNoWidget;

    EXPECT_FALSE(probe.isEntityListHovered());

    probe.pointer.hoveredWidget = antwika::editor::kStatusBarWidget;

    EXPECT_FALSE(probe.isEntityListHovered());

    probe.pointer.hoveredWidget = antwika::editor::kEntityListPanelWidget;

    EXPECT_TRUE(probe.isEntityListHovered());

    probe.pointer.hoveredWidget = antwika::editor::kEntityListScrollWidget;

    EXPECT_TRUE(probe.isEntityListHovered());

    probe.pointer.hoveredWidget = getEntityRowWidget(4);

    EXPECT_TRUE(probe.isEntityListHovered());
}
