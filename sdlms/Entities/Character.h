#pragma once
#include "Entity.h"
#include "Core/World.h"

class Character : public Entity
{
public:
    Character(World *world, SDL_FPoint p);
    ~Character();
};
