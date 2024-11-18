#include "PlayerLogic.h"
#include "../Components/Identification.h"
#include "../Components/Physics.h"
#include "../Components/Visuals.h"
#include "../Components/Gameplay.h"
#include "../Entities/Prefabs.h"
#include "../Events/Playevents.h"

using namespace ESG; // Example Space Game
using namespace GW::INPUT; // input libs
using namespace GW::AUDIO; // audio libs
bool keyPressDetected = false;

// Define a dead zone for analog inputs to ignore small, unintended movements
const float DEAD_ZONE = 0.1f;

bool ESG::PlayerLogic::Init(std::shared_ptr<flecs::world> _game,
    std::weak_ptr<const GameConfig> _gameConfig,
    GW::INPUT::GInput _immediateInput,
    GW::INPUT::GBufferedInput _bufferedInput,
    GW::INPUT::GController _controllerInput,
    GW::AUDIO::GAudio _audioEngine,
    GW::CORE::GEventGenerator _eventPusher)
{
    // Save a handle to the ECS & game settings
    game = _game;
    gameConfig = _gameConfig;
    immediateInput = _immediateInput;
    bufferedInput = _bufferedInput;
    controllerInput = _controllerInput;
    audioEngine = _audioEngine;

    // Set the player speed directly
    float speed = 5.0f; // Set the speed value directly or retrieve it from your new source
    chargeTime = 1.0f; // Set the charge time directly or retrieve it from your new source

    // Debug statement to check the value of speed
    std::cout << "Player speed: " << speed << std::endl;

    // Debug statement to check number of matching entities
    auto count = game->filter<Player, Position, ControllerID>().count();
    std::cout << "Number of entities matching the query: " << count << std::endl;

    // Add logic for updating players
    std::cout << "Before iter loop" << std::endl;
    playerSystem = game->system<Player, Position, ControllerID>("Player System")
        .iter([this](flecs::iter it, Player*, Position* p, ControllerID* c) {
    for (auto i : it) {
        std::cout << "Iterating entity: " << i << std::endl;
        // Left-right movement
        float xaxis = 0, input = 0;
        // Use the controller/keyboard to move the player around the screen
        if (c[i].index == 0) { // Enable keyboard controls for player 1
            immediateInput.GetState(G_KEY_A, input); xaxis -= input; std::cout << "KeyPress A" << std::endl;
            immediateInput.GetState(G_KEY_D, input); xaxis += input; std::cout << "KeyPress D" << std::endl;
        }

        // Apply movement
        // Limit the player to stay within -1 to +1 NDC
        p[i].value.x = G_LARGER(p[i].value.x, -0.8f);
        p[i].value.x = G_SMALLER(p[i].value.x, +0.8f);
        std::cout << "Updated Position: (" << p[i].value.x << ")" << std::endl;

    }
    // Process any cached button events after the loop (happens multiple times per frame)
    ProcessInputEvents(it.world());
            });
    std::cout << "After iter loop" << std::endl;

    // Create an event cache for when the spacebar/'A' button is pressed
    pressEvents.Create(Max_Frame_Events); // Even 32 is probably overkill for one frame

    // Register for keyboard and controller events
    bufferedInput.Register(pressEvents);
    controllerInput.Register(pressEvents);

    // Create the on explode handler
    onExplode.Create([this](const GW::GEvent& e) {
        ESG::PLAY_EVENT event; ESG::PLAY_EVENT_DATA eventData;
    if (+e.Read(event, eventData)) {
        // Only in here if event matches
        std::cout << "Enemy Was Destroyed!\n";
    }
        });
    _eventPusher.Register(onExplode);

    return true;
}

// Free any resources used to run this system
bool ESG::PlayerLogic::Shutdown()
{
    playerSystem.destruct();
    game.reset();
    gameConfig.reset();

    return true;
}

// Toggle if a system's Logic is actively running
bool ESG::PlayerLogic::Activate(bool runSystem)
{
    if (playerSystem.is_alive()) {
        (runSystem) ?
            playerSystem.enable()
            : playerSystem.disable();
        return true;
    }
    return false;
}

bool ESG::PlayerLogic::ProcessInputEvents(flecs::world& stage)
{
    // pull any waiting events from the event cache and process them
    GW::GEvent event;
    while (+pressEvents.Pop(event)) {
        bool fire = false;
        GController::Events controller;
        GController::EVENT_DATA c_data;
        GBufferedInput::Events keyboard;
        GBufferedInput::EVENT_DATA k_data;
        // these will only happen when needed
        if (+event.Read(keyboard, k_data)) {
            if (keyboard == GBufferedInput::Events::KEYPRESSED) {
                if (k_data.data == G_KEY_SPACE) {
                    fire = true;
                    chargeStart = stage.time();
                }
            }
            if (keyboard == GBufferedInput::Events::KEYRELEASED) {
                if (k_data.data == G_KEY_SPACE) {
                    chargeEnd = stage.time();
                    if (chargeEnd - chargeStart >= chargeTime) {
                        fire = true;
                    }
                }
            }
        }
        else if (+event.Read(controller, c_data)) {
            if (controller == GController::Events::CONTROLLERBUTTONVALUECHANGED) {
                if (c_data.inputValue > 0 && c_data.inputCode == G_SOUTH_BTN)
                    fire = true;
            }
        }
        if (fire) {
            // grab player one and set them to a firing state
            stage.entity("anvil_low").add<Firing>();
            std::cout << "It is Firing" << std::endl;
        }
    }
    return true;
}

// play sound and launch two laser rounds
bool ESG::PlayerLogic::FireLasers(flecs::world& stage, Position origin)
{
    // Grab the prefab for a laser round
    flecs::entity bullet;
    RetreivePrefab("Lazer Bullet", bullet);

    origin.value.x -= 0.05f;
    auto laserLeft = stage.entity().is_a(bullet)
        .set<Position>(origin);
    origin.value.x += 0.1f;
    auto laserRight = stage.entity().is_a(bullet)
        .set<Position>(origin);
    // if this shot is charged
    if (chargeEnd - chargeStart >= chargeTime) {
        chargeEnd = chargeStart;
        laserLeft.set<ChargedShot>({ 2 })
            .set<Material>({ 1,0,0 });
        laserRight.set<ChargedShot>({ 2 })
            .set<Material>({ 1,0,0 });
    }

    // play the sound of the Lazer prefab
    GW::AUDIO::GSound shoot = *bullet.get<GW::AUDIO::GSound>();
    shoot.Play();

    return true;
}


//// Grab init settings for players
//std::shared_ptr<const GameConfig> readCfg = _gameConfig.lock();
//// color
//float red = (readCfg).at("Player1").at("red").as<float>();
//float green = (readCfg).at("Player1").at("green").as<float>();
//float blue = (readCfg).at("Player1").at("blue").as<float>();
//// start position
//float xstart = (readCfg).at("Player1").at("xstart").as<float>();
//float ystart = (readCfg).at("Player1").at("ystart").as<float>();
//float scale = (readCfg).at("Player1").at("scale").as<float>();
//// Connects logic to traverse any players and allow a controller to manipulate them
//bool ESG::PlayerLogic::Init(std::shared_ptr<flecs::world> _game,
//    std::weak_ptr<const GameConfig> _gameConfig,
//    GW::INPUT::GInput _immediateInput,
//    GW::INPUT::GBufferedInput _bufferedInput,
//    GW::INPUT::GController _controllerInput,
//    GW::AUDIO::GAudio _audioEngine,
//    GW::CORE::GEventGenerator _eventPusher)
//{
//    // save a handle to the ECS & game settings
//    game = _game;
//    gameConfig = _gameConfig;
//    immediateInput = _immediateInput;
//    bufferedInput = _bufferedInput;
//    controllerInput = _controllerInput;
//    audioEngine = _audioEngine;
//    // Init any helper systems required for this task
//    std::shared_ptr<const GameConfig> readCfg = gameConfig.lock();
//    int width = (readCfg).at("Window").at("width").as<int>();
//    float speed = (readCfg).at("Player1").at("speed").as<float>();
//    chargeTime = (readCfg).at("Player1").at("chargeTime").as<float>();
//    // add logic for updating players
//    playerSystem = game->system<Player, Position, ControllerID>("Player System")
//        .iter([this, speed](flecs::iter it, Player, Position* p, ControllerID* c) {
//
//        for (auto i : it) {
//            // left-right movement
//            float xaxis = 0, input = 0;
//            // Use the controller/keyboard to move the player around the screen
//            if (c[i].index == 0) { // enable keyboard controls for player 1
//                immediateInput.GetState(G_KEY_LEFT, input); xaxis -= input;
//                immediateInput.GetState(G_KEY_RIGHT, input); xaxis += input;
//            }
//            // grab left-thumb stick
//            controllerInput.GetState(c[i].index, G_LX_AXIS, input); xaxis += input;
//            controllerInput.GetState(c[i].index, G_DPAD_LEFT_BTN, input); xaxis -= input;
//            controllerInput.GetState(c[i].index, G_DPAD_RIGHT_BTN, input); xaxis += input;
//            xaxis = G_LARGER(xaxis, -1);// cap right motion
//            xaxis = G_SMALLER(xaxis, 1);// cap left motion
//
//            // apply movement
//            p[i].value.x += xaxis * it.delta_time() * speed;
//            // limit the player to stay within -1 to +1 NDC
//            p[i].value.x = G_LARGER(p[i].value.x, -0.8f);
//            p[i].value.x = G_SMALLER(p[i].value.x, +0.8f);
//
//            // fire weapon if they are in a firing state
//            if (it.entity(i).has<Firing>()) {
//                Position offset = p[i];
//                offset.value.y += 0.05f;
//                FireLasers(it.world(), offset);
//                it.entity(i).remove<Firing>();
//            }
//        }
//        // process any cached button events after the loop (happens multiple times per frame)
//        ProcessInputEvents(it.world());
//            });
//
//    // Create an event cache for when the spacebar/'A' button is pressed
//    pressEvents.Create(Max_Frame_Events); // even 32 is probably overkill for one frame
//
//    // register for keyboard and controller events
//    bufferedInput.Register(pressEvents);
//    controllerInput.Register(pressEvents);
//
//    // create the on explode handler
//    onExplode.Create([this](const GW::GEvent& e) {
//        ESG::PLAY_EVENT event; ESG::PLAY_EVENT_DATA eventData;
//        if (+e.Read(event, eventData)) {
//            // only in here if event matches
//            std::cout << "Enemy Was Destroyed!\n";
//        }
//        });
//    _eventPusher.Register(onExplode);
//
//    return true;
//}
//
//
//// Connects logic to traverse any players and allow a controller to manipulate them
//bool ESG::PlayerLogic::Init(std::shared_ptr<flecs::world> _game,
//    std::weak_ptr<const GameConfig> _gameConfig,
//    GW::INPUT::GInput _immediateInput,
//    GW::INPUT::GBufferedInput _bufferedInput,
//    GW::INPUT::GController _controllerInput,
//    GW::AUDIO::GAudio _audioEngine,
//    GW::CORE::GEventGenerator _eventPusher)
//{
//    // save a handle to the ECS & game settings
//    game = _game;
//    gameConfig = _gameConfig;
//    immediateInput = _immediateInput;
//    bufferedInput = _bufferedInput;
//    controllerInput = _controllerInput;
//    audioEngine = _audioEngine;
//    // Init any helper systems required for this task
//    std::shared_ptr<const GameConfig> readCfg = gameConfig.lock();
//    int width = (*readCfg).at("Window").at("width").as<int>();
//    float speed = (*readCfg).at("Player1").at("speed").as<float>();
//    chargeTime = (*readCfg).at("Player1").at("chargeTime").as<float>();
//
//    auto e = _game->lookup("anvil_low");
//    if (e.is_valid()) {
//        std::cout << "Player 1 Found" << std::endl;
//    }
//
//    // add logic for updating players
//    playerSystem = game->system<Player, Position, ControllerID>("Player System")
//        .iter([this, speed](flecs::iter it, Player*, Position* p, ControllerID* c) {
//
//        for (auto i : it) {
//            // left-right movement
//            float xaxis = 0, input = 0;
//
//            // Use the controller/keyboard to move the player around the screen            
//            if (c[i].index == 0) { // enable keyboard controls for player 1
//                immediateInput.GetState(G_KEY_LEFT, input);
//                if (input != 0) {
//                    std::cout << "Controller input: " << input << ", xaxis: " << xaxis << std::endl;
//                    xaxis -= input; keyPressDetected = true;
//                }
//                immediateInput.GetState(G_KEY_RIGHT, input);
//                if (input != 0) { xaxis += input; keyPressDetected = true; }
//            }
//            // grab left-thumb stick
//            controllerInput.GetState(c[i].index, G_LX_AXIS, input);
//            if (input != 0) { xaxis += input; keyPressDetected = true; }
//            controllerInput.GetState(c[i].index, G_DPAD_LEFT_BTN, input);
//            if (input != 0) { xaxis -= input; keyPressDetected = true; }
//            controllerInput.GetState(c[i].index, G_DPAD_RIGHT_BTN, input);
//            if (input != 0) { xaxis += input; keyPressDetected = true; }
//
//            // Log the input values for debugging
//            if (keyPressDetected) {
//                std::cout << "Controller input: " << input << ", xaxis: " << xaxis << std::endl;
//            }
//
//            // Apply dead zone check
//            if (std::abs(xaxis) < DEAD_ZONE) {
//                xaxis = 0;
//            }
//
//            auto e = game->lookup("anvil_low");
//            if (e.is_valid()) {
//                std::cout << "Player 2 Found" << std::endl;
//
//                // Use the controller/keyboard to move the player around the screen            
//                if (c[e].index == 0) { // enable keyboard controls for player 1
//                    immediateInput.GetState(G_KEY_LEFT, input);
//                    if (input != 0) {
//                        xaxis -= input; keyPressDetected = true;
//                    }
//                    immediateInput.GetState(G_KEY_RIGHT, input);
//                    if (input != 0) { xaxis += input; keyPressDetected = true; }
//                }
//
//                // apply movement
//                float previousX = p[e].value.x;
//                // limit the player to stay within -1 to +1 NDC
//                p[e].value.x = G_LARGER(p[e].value.x, -0.8f);
//                p[e].value.x = G_SMALLER(p[e].value.x, +0.8f);
//
//                // Log position updates only if they have changed
//                if (previousX != p[e].value.x) {
//                    std::cout << "Player position updated to: (" << p[e].value.x << ", " << p[e].value.y << ", " << p[e].value.z << ")" << std::endl;
//                    std::cout << "Previous position: " << previousX << ", New position: " << p[e].value.x << std::endl;
//                }
//
//            }
//
//
//            // fire weapon if they are in a firing state
//            if (it.entity(i).has<Firing>()) {
//                Position offset = p[i];
//                offset.value.y += 0.05f;
//                FireLasers(it.world(), offset);
//                it.entity(i).remove<Firing>();
//            }
//        }
//        // process any cached button events after the loop (happens multiple times per frame)
//        ProcessInputEvents(it.world());
//            });
//
//    // Create an event cache for when the spacebar/'A' button is pressed
//    pressEvents.Create(Max_Frame_Events); // even 32 is probably overkill for one frame
//
//    // register for keyboard and controller events
//    bufferedInput.Register(pressEvents);
//    controllerInput.Register(pressEvents);
//
//    // create the on explode handler
//    onExplode.Create([this](const GW::GEvent& e) {
//        ESG::PLAY_EVENT event; ESG::PLAY_EVENT_DATA eventData;
//        if (+e.Read(event, eventData)) {
//            // only in here if event matches
//            std::cout << "Enemy Was Destroyed!\n";
//        }
//        });
//    _eventPusher.Register(onExplode);
//
//
//    return true;
//}