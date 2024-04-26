#pragma once
#include "Systems/System.h"
#include "Components/Sprite.h"
#include "Components/Transform.h"


class RenderSystem : public System {
public:
	void run(World& world) override;

private:
	void render_sprite(Sprite* spr, Transform* tr, World& world);
};