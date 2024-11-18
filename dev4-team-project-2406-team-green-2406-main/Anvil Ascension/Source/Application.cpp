#include "Application.h"
#include "Systems/load_data_oriented.h"
#include "Systems/load_object_oriented.h"
#include "Systems/OpenGLExtensions.h"

// Open some Gateware namespaces for convenience 
// NEVER do this in a header file!
using namespace GW;
using namespace CORE;
using namespace SYSTEM;
using namespace GRAPHICS;

bool Application::Init()
{
    eventPusher.Create();

    // Load all game settings
    gameConfig = std::make_shared<GameConfig>();
    // Create the ECS system
    game = std::make_shared<flecs::world>();
    // Init all other systems
    if (InitWindow() == false) {
        std::cerr << "Failed to initialize window." << std::endl;
        return false;
    }
    if (InitInput() == false) {
        std::cerr << "Failed to initialize input." << std::endl;
        return false;
    }
    if (InitAudio() == false) {
        std::cerr << "Failed to initialize audio." << std::endl;
        return false;
    }
    if (InitGraphics() == false) {
        std::cerr << "Failed to initialize graphics." << std::endl;
        return false;
    }
    if (InitEntities() == false) {
        std::cerr << "Failed to initialize entities." << std::endl;
        return false;
    }
    if (InitSystems() == false) {
        std::cerr << "Failed to initialize systems." << std::endl;
        return false;
    }

    std::cout << "Initialization complete." << std::endl;
    return true;
}

bool Application::Run() {
    std::cout << "Starting Run loop..." << std::endl;

    // Initialize and set up the window
    GEventResponder msgs;
    GOpenGLSurface ogl;
    GW::SYSTEM::GLog log;

    // Create the OpenGL surface and set up event handling
    if (+ogl.Create(window, GW::GRAPHICS::DEPTH_BUFFER_SUPPORT)) {
        QueryOGLExtensionFunctions(ogl); // Link needed OpenGL API functions
        Level_Objects objectOrientedLoader(window, ogl);
        objectOrientedLoader.LoadLevel("../GameLevel.txt", "../Models", log);
        objectOrientedLoader.UploadLevelToGPU();

        GLuint shaderProgram = objectOrientedLoader.GetShaderProgram();
        float clr[] = { 0 / 255.0f, 0 / 255.0f, 0 / 255.0f, 1 };

        std::cout << "Entering main loop..." << std::endl;

        // Main loop
        while (+window.ProcessWindowEvents()) {

            float xchangeleft, xchangeright;
            float input = 0;
            const ESG::Position* pos = game->entity("anvil_low").get<ESG::Position>();
            auto e = game->lookup("anvil_low");
            if (e.is_valid())
            {

            }

            glClearColor(clr[0], clr[1], clr[2], clr[3]);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            static auto start = std::chrono::steady_clock::now();
            double elapsed = std::chrono::duration<double>(
                std::chrono::steady_clock::now() - start).count();
            start = std::chrono::steady_clock::now();

            // Update and render the level
            objectOrientedLoader.UpdateAndRender(elapsed);

            ogl.UniversalSwapBuffers();
        }
        std::cout << "Exiting main loop." << std::endl;
        return true;
    }
    std::cerr << "Failed to create OpenGL surface." << std::endl;
    return false;
}

bool Application::Shutdown()
{
    // Disconnect systems from global ECS
    if (playerSystem.Shutdown() == false) {
        std::cerr << "Failed to shut down player system." << std::endl;
        return false;
    }
    if (levelSystem.Shutdown() == false) {
        std::cerr << "Failed to shut down level system." << std::endl;
        return false;
    }
    if (physicsSystem.Shutdown() == false) {
        std::cerr << "Failed to shut down physics system." << std::endl;
        return false;
    }
    if (bulletSystem.Shutdown() == false) {
        std::cerr << "Failed to shut down bullet system." << std::endl;
        return false;
    }
    if (enemySystem.Shutdown() == false) {
        std::cerr << "Failed to shut down enemy system." << std::endl;
        return false;
    }

    std::cout << "Shutdown complete." << std::endl;
    return true;
}

bool Application::InitWindow() {
    GEventResponder msgs;
    GOpenGLSurface ogl;
    GW::SYSTEM::GLog log;

    log.Create("../LevelLoaderLog.txt");
    log.EnableConsoleLogging(true); // Mirror output to the console
    log.Log("Start Program.");

    Level_Data dataOrientedLoader;
    dataOrientedLoader.LoadLevel("../GameLevel.txt", "../Models", log);

    std::cout << "Window settings loaded from gameConfig." << std::endl;

    // Grab settings
    int width = gameConfig->at("Window").at("width").as<int>();
    int height = gameConfig->at("Window").at("height").as<int>();
    int xstart = gameConfig->at("Window").at("xstart").as<int>();
    int ystart = gameConfig->at("Window").at("ystart").as<int>();
    std::string title = gameConfig->at("Window").at("title").as<std::string>();

    // Open window
    if (+window.Create(xstart, ystart, width, height, GWindowStyle::WINDOWEDBORDERED) &&
        +window.SetWindowName(title.c_str())) {
        std::cout << "Window created successfully." << std::endl;
        return true;
    }
    std::cerr << "Failed to create window." << std::endl;
    return false;
}

bool Application::InitInput()
{
    if (-gamePads.Create()) {
        std::cerr << "Failed to create game pads." << std::endl;
        return false;
    }
    if (-immediateInput.Create(window)) {
        std::cerr << "Failed to create immediate input." << std::endl;
        return false;
    }
    if (-bufferedInput.Create(window)) {
        std::cerr << "Failed to create buffered input." << std::endl;
        return false;
    }
    std::cout << "Input initialized successfully." << std::endl;
    return true;
}

bool Application::InitAudio()
{
    if (-audioEngine.Create()) {
        std::cerr << "Failed to create audio engine." << std::endl;
        return false;
    }
    std::cout << "Audio initialized successfully." << std::endl;
    return true;
}

bool Application::InitEntities()
{
    // Load bullet prefabs
    if (weapons.Load(game, gameConfig, audioEngine) == false) {
        std::cerr << "Failed to load bullet prefabs." << std::endl;
        return false;
    }
    // Load the player entities
    if (players.Load(game, gameConfig) == false) {
        std::cerr << "Failed to load player entities." << std::endl;
        return false;
    }
    // Load the enemy entities
    if (enemies.Load(game, gameConfig, audioEngine) == false) {
        std::cerr << "Failed to load enemy entities." << std::endl;
        return false;
    }
    std::cout << "Entities initialized successfully." << std::endl;
    return true;
}

bool Application::InitSystems()
{
    // Connect systems to global ECS
    if (playerSystem.Init(game, gameConfig, immediateInput, bufferedInput,
        gamePads, audioEngine, eventPusher) == false) {
        std::cerr << "Failed to initialize player system." << std::endl;
        return false;
    }
    if (levelSystem.Init(game, gameConfig, audioEngine) == false) {
        std::cerr << "Failed to initialize level system." << std::endl;
        return false;
    }
    if (physicsSystem.Init(game, gameConfig) == false) {
        std::cerr << "Failed to initialize physics system." << std::endl;
        return false;
    }
    if (bulletSystem.Init(game, gameConfig) == false) {
        std::cerr << "Failed to initialize bullet system." << std::endl;
        return false;
    }
    if (enemySystem.Init(game, gameConfig, eventPusher) == false) {
        std::cerr << "Failed to initialize enemy system." << std::endl;
        return false;
    }
    std::cout << "Systems initialized successfully." << std::endl;
    return true;
}

bool Application::InitGraphics() {
    GEventResponder msgs;
    GOpenGLSurface ogl;
    GW::SYSTEM::GLog log;

    log.Create("../LevelLoaderLog.txt");
    log.EnableConsoleLogging(true); // Mirror output to the console
    log.Log("Starting graphics initialization...");

    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    float clr[] = { 0 / 255.0f, 0 / 255.0f, 0 / 255.0f, 1 };

    // Create the message handler
    msgs.Create([&](const GW::GEvent& e) {
        GW::SYSTEM::GWindow::Events q;
    if (+e.Read(q) && q == GWindow::Events::RESIZE)
        ; // Do nothing or handle resize if needed
        });
    window.Register(msgs);

    log.Log("Attempting to create OpenGL surface...");

    if (+ogl.Create(window, GW::GRAPHICS::DEPTH_BUFFER_SUPPORT)) {
        log.Log("OpenGL surface created successfully.");
        QueryOGLExtensionFunctions(ogl); // Link needed OpenGL API functions

        return true;
    }
    log.Log("Failed to create OpenGL surface.");
    return false;
}

bool Application::GameLoop()
{
    // Compute delta time and pass to the ECS system
    static auto start = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - start).count();
    start = std::chrono::steady_clock::now();

    // Let the ECS system run
    return game->progress(static_cast<float>(elapsed));
}


//// grab settings
//int width = gameConfig->at("Window").at("width").as<int>();
//int height = gameConfig->at("Window").at("height").as<int>();
//int xstart = gameConfig->at("Window").at("xstart").as<int>();
//int ystart = gameConfig->at("Window").at("ystart").as<int>();
//std::string title = gameConfig->at("Window").at("title").as<std::string>();
//// open window
//if (+window.Create(xstart, ystart, width, height, GWindowStyle::WINDOWEDLOCKED) &&
//	+window.SetWindowName(title.c_str())) {
//	return true;
//}
//return false;