#pragma once

#include <cstddef>
#include <deque>
#include <optional>
#include <utility>

namespace antwika::map
{

    inline constexpr std::size_t kDefaultHistoryDepth = 64;

    template <
        typename State,
        std::size_t Depth = kDefaultHistoryDepth>
    class History final
    {
    public:
        void push(State currentState)
        {
            undoStack.push_back(std::move(currentState));
            redoStack.clear();

            while (undoStack.size() > Depth)
            {
                undoStack.pop_front();
            }
        }

        [[nodiscard]] std::optional<State> undo(State currentState)
        {
            if (undoStack.empty())
            {
                return std::nullopt;
            }

            auto undoneState = std::move(undoStack.back());

            undoStack.pop_back();
            redoStack.push_back(std::move(currentState));

            return undoneState;
        }

        [[nodiscard]] std::optional<State> redo(State currentState)
        {
            if (redoStack.empty())
            {
                return std::nullopt;
            }

            auto redoneState = std::move(redoStack.back());

            redoStack.pop_back();
            undoStack.push_back(std::move(currentState));

            return redoneState;
        }

        [[nodiscard]] std::size_t undoCount() const
        {
            return undoStack.size();
        }

        [[nodiscard]] std::size_t redoCount() const
        {
            return redoStack.size();
        }

        void clear()
        {
            undoStack.clear();
            redoStack.clear();
        }

    private:
        std::deque<State> undoStack{};

        std::deque<State> redoStack{};
    };

}
