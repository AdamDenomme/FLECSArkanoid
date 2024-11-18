// The Enemy system is responsible for enemy behaviors
#ifndef ENEMYLOGIC_H
#define ENEMYLOGIC_H

// Contains our global game settings
#include "../GameConfig.h"
#include "../Entities/EnemyData.h"

// example space game (avoid name collisions)
namespace ESG
{
	class EnemyLogic
	{
		// shared connection to the main ECS engine
		std::shared_ptr<flecs::world> game;
		// non-ownership handle to configuration settings
		std::weak_ptr<const GameConfig> gameConfig;
		// handle to events
		GW::CORE::GEventGenerator eventPusher;
	public:
		// attach the required logic to the ECS 
		bool Init(std::shared_ptr<flecs::world> _game,
			std::weak_ptr<const GameConfig> _gameConfig,
			GW::CORE::GEventGenerator _eventPusher);
		// control if the system is actively running
		bool Activate(bool runSystem);
		// release any resources allocated by the system
		bool Shutdown();
	};

};

#endif