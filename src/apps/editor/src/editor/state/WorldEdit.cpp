#include "antwika/editor/editor/state/WorldEdit.hpp"

namespace antwika::editor
{

    std::int32_t WorldEdit::getEditLevel() const noexcept
    {
        return editLevel;
    }

    void WorldEdit::setEditLevel(const std::int32_t level) noexcept
    {
        editLevel = level;
    }

    void WorldEdit::stepLevelUp() noexcept
    {
        editLevel += 1;
    }

    void WorldEdit::stepLevelDown() noexcept
    {
        editLevel -= 1;
    }

    solver::CornerSeams WorldEdit::getCornerJoining() const noexcept
    {
        return cornerJoining;
    }

    bool WorldEdit::isCornerJoiningOn() const noexcept
    {
        return cornerJoining == solver::CornerSeams::Included;
    }

    void WorldEdit::toggleCornerJoining() noexcept
    {
        cornerJoining = cornerJoining == solver::CornerSeams::Included
                            ? solver::CornerSeams::Ignored
                            : solver::CornerSeams::Included;
    }

    void WorldEdit::setCornerJoining(const bool cornersJoined) noexcept
    {
        cornerJoining = cornersJoined ? solver::CornerSeams::Included
                                      : solver::CornerSeams::Ignored;
    }

    float WorldEdit::getRiseAxis() const noexcept
    {
        return (ascendHeld ? 1.0F : 0.0F) - (descendHeld ? 1.0F : 0.0F);
    }

    bool WorldEdit::isRiseHeld() const noexcept
    {
        return descendHeld || ascendHeld;
    }

    void WorldEdit::setAscendHeld(const bool held) noexcept
    {
        ascendHeld = held;
    }

    void WorldEdit::setDescendHeld(const bool held) noexcept
    {
        descendHeld = held;
    }

}
