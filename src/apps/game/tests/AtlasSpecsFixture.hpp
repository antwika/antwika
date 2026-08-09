#pragma once

#include "antwika/game/TileAtlas.hpp"

namespace antwika::game::testing
{

    inline constexpr AtlasSpecs kTestSpecs{
        {{{.spriteSize = {.width = 64, .height = 96},
           .pivot = {.x = 32, .y = 64},
           .isometric = {.width = 32, .height = 16},
           .columns = 8,
           .rows = 8},
          {.spriteSize = {.width = 96, .height = 112},
           .pivot = {.x = 48, .y = 80},
           .isometric = {.width = 64, .height = 32},
           .columns = 8,
           .rows = 8},
          {.spriteSize = {.width = 128, .height = 128},
           .pivot = {.x = 64, .y = 96},
           .isometric = {.width = 96, .height = 48},
           .columns = 8,
           .rows = 8}}},
        {.spriteSize = {.width = 64, .height = 96},
         .pivot = {.x = 32, .y = 64},
         .isometric = {.width = 32, .height = 16},
         .columns = 8,
         .rows = 8}};

}
