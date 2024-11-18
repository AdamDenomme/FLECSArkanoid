#ifndef APPLICATION_H
#define APPLICATION_H

// include events
#include "Events/Playevents.h"
// Contains our global game settings
#include "GameConfig.h"
// Load all entities+prefabs used by the game 
#include "Entities/BulletData.h"
#include "Entities/PlayerData.h"
#include "Entities/EnemyData.h"
// Include all systems used by the game and their associated components
#include "Systems/PlayerLogic.h"
#include "Systems/LevelLogic.h"
#include "Systems/PhysicsLogic.h"
#include "Systems/BulletLogic.h"
#include "Systems/EnemyLogic.h"

// Allocates and runs all sub-systems essential to operating the game
class Application 
{
	// gateware libs used to access operating system
	GW::SYSTEM::GWindow window; // gateware multi-platform window
	GW::INPUT::GController gamePads; // controller support
	GW::INPUT::GInput immediateInput; // twitch keybaord/mouse
	GW::INPUT::GBufferedInput bufferedInput; // event keyboard/mouse
	GW::AUDIO::GAudio audioEngine; // can create music & sound effects
	// third-party gameplay & utility libraries
	std::shared_ptr<flecs::world> game; // ECS database for gameplay
	std::shared_ptr<GameConfig> gameConfig; // .ini file game settings
	// ECS Entities and Prefabs that need to be loaded
	ESG::BulletData weapons;
	ESG::PlayerData players;
	ESG::EnemyData enemies;
	// specific ECS systems used to run the game
	ESG::PlayerLogic playerSystem;
	ESG::LevelLogic levelSystem;
	ESG::PhysicsLogic physicsSystem;
	ESG::BulletLogic bulletSystem;
	ESG::EnemyLogic enemySystem;
	// EventGenerator for Game Events
	GW::CORE::GEventGenerator eventPusher;

public:
	bool Init();
	bool Run();
	bool Shutdown();
private:
	bool InitWindow();
	bool InitInput();
	bool InitAudio();
	bool InitGraphics();
	bool InitEntities();
	bool InitSystems();
	bool GameLoop();
};

#endif 