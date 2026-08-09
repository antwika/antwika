#pragma once

namespace antwika::game
{

    class IMenuCommands
    {
    public:
        virtual ~IMenuCommands() = default;

        virtual void newGame() = 0;

        virtual void openSaves() = 0;

        virtual void mainMenu() = 0;

        virtual void worldMap() = 0;
    };

}
