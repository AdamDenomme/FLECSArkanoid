#include "PlayerData.h"
#include "../Components/Identification.h"
#include "../Components/Visuals.h"
#include "../Components/Physics.h"
#include "Prefabs.h"
#include <regex>

bool ESG::PlayerData::ParseGameLevelFile(const std::string& filePath, std::string& modelName, float& xstart, float& ystart, float& zstart, float& scale) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open game level file: " << filePath << std::endl;
        return false;
    }

    std::string line;
    GW::MATH::GMATRIXF transform;
    while (std::getline(file, line)) {
        if (line.find("MESH") != std::string::npos) {
            std::getline(file, modelName);
            if (modelName == "anvil_low") {
                std::cout << "Found model: " << modelName << std::endl; // Log model name
                // Read the transformation matrix
                for (int i = 0; i < 4; ++i) {
                    std::getline(file, line);
                    std::sscanf(line.c_str() + 13, "%f, %f, %f, %f",
                        &transform.data[0 + i * 4], &transform.data[1 + i * 4],
                        &transform.data[2 + i * 4], &transform.data[3 + i * 4]);
                }
                // Extract position
                xstart = transform.row4.x;
                ystart = transform.row4.y;
                zstart = transform.row4.z;

                // Calculate scale from the matrix columns
                float scaleX = std::sqrt(transform.data[0] * transform.data[0] +
                    transform.data[4] * transform.data[4] +
                    transform.data[8] * transform.data[8]);
                float scaleY = std::sqrt(transform.data[1] * transform.data[1] +
                    transform.data[5] * transform.data[5] +
                    transform.data[9] * transform.data[9]);
                float scaleZ = std::sqrt(transform.data[2] * transform.data[2] +
                    transform.data[6] * transform.data[6] +
                    transform.data[10] * transform.data[10]);
                scale = (scaleX + scaleY + scaleZ) / 3.0f;

                std::cout << "Position: (" << xstart << ", " << ystart << ", " << zstart << "), Scale: " << scale << std::endl; // Log position and scale
                file.close();
                return true;
            }
        }
    }

    std::cerr << "Model anvil_low.h2b not found in game level file." << std::endl;
    file.close();
    return false;
}

bool ESG::PlayerData::Load(std::shared_ptr<flecs::world> _game, std::weak_ptr<const GameConfig> _gameConfig) {
    std::shared_ptr<const GameConfig> readCfg = _gameConfig.lock();
    std::string gameLevelPath = (*readCfg).at("Level1").at("gamelevel").as<std::string>();

    std::cout << "Loading game level from: " << gameLevelPath << std::endl; // Log game level path

    std::string modelName;
    float xstart, ystart, zstart, scale;
    if (!ParseGameLevelFile(gameLevelPath, modelName, xstart, ystart, zstart, scale)) {
        std::cerr << "Failed to parse game level file or model anvil_low.h2b not found." << std::endl; // Log error
        return false;
    }

    // Log extracted values
    std::cout << "Model: " << modelName << ", Position: (" << xstart << ", " << ystart << ", " << zstart << "), Scale: " << scale << std::endl;
  

    auto entity = _game->entity("anvil_low")
        .set<Position>({ xstart, ystart, zstart }) // Set the initial position
        .set<ControllerID>({ 0 }) // Assuming controller index 0 for player 1
        .add<Player>(); // Add the Player component

    // Check if the entity has the required components
    if (entity.has<Player>() && entity.has<ControllerID>() && entity.has<Position>()) {
        std::cout << "Entity anvil_low created successfully with Player, ControllerID, and Position components." << std::endl;
    }
    else {
        std::cerr << "Failed to add required components to entity anvil_low." << std::endl;
        return false;
    }

    return true;
}

bool ESG::PlayerData::Unload(std::shared_ptr<flecs::world> _game)
{
	// remove all players
	_game->defer_begin(); // required when removing while iterating!
		_game->each([](flecs::entity e, Player&) {
			e.destruct(); // destroy this entitiy (happens at frame end)
		});
	_game->defer_end(); // required when removing while iterating!

    return true;
}


//float red = (*readCfg).at("Player1").at("red").as<float>();
//float green = (*readCfg).at("Player1").at("green").as<float>();
//float blue = (*readCfg).at("Player1").at("blue").as<float>();