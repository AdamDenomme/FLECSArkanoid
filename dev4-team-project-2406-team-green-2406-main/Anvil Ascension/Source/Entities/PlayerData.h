// This class populates all player entities 
#ifndef PLAYERDATA_H
#define PLAYERDATA_H

// Contains our global game settings
#include "../GameConfig.h"


// example space game (avoid name collisions)
namespace ESG
{
	class PlayerData
	{
	public:
		bool ESG::PlayerData::ParseGameLevelFile(const std::string& filePath, std::string& modelName, float& xstart, float& ystart, float& zstart, float& scale);
		// Load required entities and/or prefabs into the ECS 
		bool Load(	std::shared_ptr<flecs::world> _game,
					std::weak_ptr<const GameConfig> _gameConfig);
		// Unload the entities/prefabs from the ECS
		bool Unload(std::shared_ptr<flecs::world> _game);
	};

};

#endif