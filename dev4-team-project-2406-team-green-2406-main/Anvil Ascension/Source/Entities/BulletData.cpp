#include "BulletData.h"
#include "../Components/Identification.h"
#include "../Components/Visuals.h"
#include "../Components/Physics.h"
#include "Prefabs.h"
#include "../Components/Gameplay.h"
#include <iostream> // Include for debug statements

bool ESG::BulletData::Load(std::shared_ptr<flecs::world> _game,
    std::weak_ptr<const GameConfig> _gameConfig,
    GW::AUDIO::GAudio _audioEngine)
{
    // Grab init settings for the ball
    std::shared_ptr<const GameConfig> readCfg = _gameConfig.lock();

    // Create entity for ball
    // other attributes
    float speed = (*readCfg).at("Ball").at("speed").as<float>();
    float scale = (*readCfg).at("Ball").at("scale").as<float>();
    int dmg = (*readCfg).at("Ball").at("damage").as<int>();
    std::string fireFX = (*readCfg).at("Ball").at("fireFX").as<std::string>();
    std::string modelPath = (*readCfg).at("Ball").at("Model").as<std::string>();
    float xstart = (*readCfg).at("Ball").at("xstart").as<float>();
    float ystart = (*readCfg).at("Ball").at("ystart").as<float>();
    float zstart = (*readCfg).at("Ball").at("zstart").as<float>();

    // default ball scale
    GW::MATH::GMATRIXF world;
    GW::MATH::GMatrix::ScaleLocalF(GW::MATH::GIdentityMatrixF, GW::MATH::GVECTORF{ scale, scale, scale }, world);

    // Load sound effect used by this ball entity
    GW::AUDIO::GSound shoot;
    shoot.Create(fireFX.c_str(), _audioEngine, 0.15f); // we need a global music & sfx volumes

    // add ball entity to ECS
    auto ballEntity = _game->entity("Ball")
        .set<Model>({ modelPath })
        .set<Orientation>({ world })
        .set<Acceleration>({ 0, 0, 0 }) // Adjusted for 3D
        .set<Velocity>({ 0, speed, 0 }) // Adjusted for 3D
        .set<GW::AUDIO::GSound>(shoot.Relinquish())
        .set<Damage>({ dmg })
        .set<Position>({ xstart, ystart, zstart }) // Set initial position, adjust as necessary
        .add<Bullet>();

    
    // Debug statement to confirm ball loading
    std::cout << "Ball entity created with the following attributes:\n";
    std::cout << "Model Path: " << modelPath << "\n";
    std::cout << "Scale: " << scale << "\n";
    std::cout << "Speed: " << speed << "\n";
    std::cout << "Damage: " << dmg << "\n";
    std::cout << "Fire FX: " << fireFX << "\n";
    std::cout << "Starting Position: (" << xstart << ", " << ystart << ", " << zstart << ")\n";

    return true;
}

bool ESG::BulletData::Unload(std::shared_ptr<flecs::world> _game)
{
    // remove the ball entity
    _game->defer_begin(); // required when removing while iterating!
    auto ballEntity = _game->lookup("Ball");
    if (ballEntity.is_alive()) {
        ballEntity.destruct(); // destroy this entity (happens at frame end)
    }
    _game->defer_end(); // required when removing while iterating!

    return true;
}
