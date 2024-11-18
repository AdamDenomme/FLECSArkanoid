#include <random>
#include "BulletLogic.h"
#include "../Components/Identification.h"
#include "../Components/Physics.h"
#include "../Components/Gameplay.h"
#include "../Components/Visuals.h"

using namespace ESG; // Example Space Game

// Connects logic to traverse any players and allow a controller to manipulate them
bool ESG::BulletLogic::Init(std::shared_ptr<flecs::world> _game,
	std::weak_ptr<const GameConfig> _gameConfig)
{
	// save a handle to the ECS & game settings
	game = _game;
	gameConfig = _gameConfig;

	// destroy any balls that have the CollidedWith relationship
	game->system<Bullet, Damage>("Bullet System")
		.each([](flecs::entity e, Bullet, Damage& d) {
		// damage anything we come into contact with
		e.each<CollidedWith>([&e, d](flecs::entity hit) {
				if (hit.has<Health>()) {
					int current = hit.get<Health>()->value;
					hit.set<Health>({ current - d.value });
					// reduce the amount of hits but the charged shot
					if (e.has<ChargedShot>() && hit.get<Health>()->value <= 0)
					{
						int md_count = e.get<ChargedShot>()->max_destroy;
						e.set<ChargedShot>({ md_count - 1 });
					}
				}
			});
	// if you have collidedWith relationship then be destroyed
	if (e.has<CollidedWith>(flecs::Wildcard)) {

		if (e.has<ChargedShot>()) {

			if (e.get<ChargedShot>()->max_destroy <= 0)
				e.destruct();
		}
		else {
			// play hit sound
			e.destruct();
		}
	}
			});

	return true;
}

// Free any resources used to run this system
bool ESG::BulletLogic::Shutdown()
{
	game->entity("Bullet System").destruct();
	// invalidate the shared pointers
	game.reset();
	gameConfig.reset();
	return true;
}

// Toggle if a system's Logic is actively running
bool ESG::BulletLogic::Activate(bool runSystem)
{
	if (runSystem) {
		game->entity("Bullet System").enable();
	}
	else {
		game->entity("Bullet System").disable();
	}
	return false;
}
