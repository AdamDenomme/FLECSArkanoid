// The level system is responsible for transitioning the various levels in the game
#ifndef LEVELLOGIC_H
#define LEVELLOGIC_H

// Contains our global game settings
#include "../GameConfig.h"
// Entities for players, enemies & bullets
#include "../Entities/PlayerData.h"
#include "../Entities/BulletData.h"

// example space game (avoid name collisions)
namespace ESG
{
	class LevelLogic
	{
		// shared connection to the main ECS engine
		std::shared_ptr<flecs::world> game;
		// async version of above for threaded operations
		flecs::world gameAsync; 
		// mutex used to protect access to gameAsync 
		GW::CORE::GThreadShared gameLock;
		// non-ownership handle to configuration settings
		std::weak_ptr<const GameConfig> gameConfig;
		// Level system will also load and switch music
		GW::AUDIO::GAudio audioEngine;
		GW::AUDIO::GMusic currentTrack;
		// Used to spawn enemies at a regular intervals on another thread
		GW::SYSTEM::GDaemon timedEvents;
	public:
		// attach the required logic to the ECS 
		bool Init(	std::shared_ptr<flecs::world> _game,
					std::weak_ptr<const GameConfig> _gameConfig,
					GW::AUDIO::GAudio _audioEngine);
		// control if the system is actively running
		bool Activate(bool runSystem);
		// release any resources allocated by the system
		bool Shutdown();
	};

};

#endif