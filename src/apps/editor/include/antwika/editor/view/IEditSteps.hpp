#pragma once

namespace antwika::editor
{

    class IEditSteps
    {
    public:
        IEditSteps() = default;

        IEditSteps(const IEditSteps &) = delete;
        IEditSteps(IEditSteps &&) = delete;

        IEditSteps &operator=(const IEditSteps &) = delete;
        IEditSteps &operator=(IEditSteps &&) = delete;

        virtual ~IEditSteps() = default;

        virtual void pushUndo() = 0;
    };

}
