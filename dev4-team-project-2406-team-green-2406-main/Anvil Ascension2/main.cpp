// Simple basecode showing how to create a window and attatch a openglsurface
#define GATEWARE_ENABLE_CORE // All libraries need this
#define GATEWARE_ENABLE_SYSTEM // Graphics libs require system level libraries
#define GATEWARE_ENABLE_GRAPHICS // Enables all Graphics Libraries
// Ignore some GRAPHICS libraries we aren't going to use
#define GATEWARE_DISABLE_GDIRECTX11SURFACE // we have another template for this
#define GATEWARE_DISABLE_GDIRECTX12SURFACE // we have another template for this
#define GATEWARE_DISABLE_GVULKANSURFACE // we have another template for this
#define GATEWARE_DISABLE_GRASTERSURFACE // we have another template for this

#define GATEWARE_ENABLE_MATH
#define GATEWARE_ENABLE_AUDIO

#define GATEWARE_ENABLE_INPUT
// With what we want & what we don't defined we can include the API
#include "gateware-main/Gateware.h"
#include "OpenGLExtensions.h"
#include "load_data_oriented.h"
#include "load_object_oriented.h"
#include "components.h"
#include "flecs-3.2.0/flecs.h"
#include "gameplay.h"
#include <cstring> 

// open some namespaces to compact the code a bit
using namespace GW;
using namespace CORE;
using namespace SYSTEM;
using namespace GRAPHICS;
using namespace INPUT;


// lets pop a window and use OpenGL to clear to a green screen
int main(GW::INPUT::GInput& input)
{
    GWindow win;
    GEventResponder msgs;
    GOpenGLSurface ogl;
    GW::SYSTEM::GLog log;
    bool dead = false;
    GW::INPUT::GInput kbm;

    GW::AUDIO::GAudio audioEngine;
    GW::AUDIO::GMusic currentTrack;

    bool previousKeyState = false;
    bool gameRunning = true;

    float toggleKey = 0;


    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float timeElapsed = std::chrono::duration<float>(currentTime - startTime).count();

    auto gameLevel = std::make_shared<Level_Data>();

    log.Create("../LevelLoaderLog.txt");
    log.EnableConsoleLogging(true); // mirror output to the console
    log.Log("Start Program.");
    if (false == gameLevel->LoadLevel("../GameLevel_1.txt", "../Models", log))
        log.LogCategorized("ERROR", "Failed to Load Game Level");
    // import level data into Gameplay engine (FLECS)
    log.LogCategorized("GAMEPLAY", "Begin injecting Blender objects as FLECS entities.");
    auto engine = std::make_unique<Gameplay>(*gameLevel, log);
    log.LogCategorized("GAMEPLAY", "Blender to FLECS injection complete.");

    std::cout << std::endl; // leave a gap to make the difference easier to see.

    if (+win.Create(0, 0, 800, 600, GWindowStyle::WINDOWEDBORDERED))
    {
        // Create the message handler
        msgs.Create([&](const GW::GEvent& e) {
            GW::SYSTEM::GWindow::Events q;
        if (+e.Read(q) && q == GWindow::Events::RESIZE)
            ; // Do nothing or handle resize if needed
            });
        win.Register(msgs);
        try
        {
            if (+kbm.Create(win))
            {
                log.Log("Input Initialized Successfully.");
            }
            else
            {
                log.LogCategorized("ERROR", "Failed to Initialize Input.");
                return -1; // Exit if input initialization fails
            }
        }
        catch (const std::exception& e)
        {
            log.LogCategorized("EXCEPTION", e.what());
            return -1; // Exit on exception
        }
        catch (...)
        {
            log.LogCategorized("EXCEPTION", "Unknown exception during GInput creation.");
            return -1; // Exit on unknown exception
        }

        win.SetWindowName("Anvil Ascension");

        if (+ogl.Create(win, GW::GRAPHICS::DEPTH_BUFFER_SUPPORT))
        {
            QueryOGLExtensionFunctions(ogl); // Link Needed OpenGL API functions
            Level_Objects objectOrientedLoader(win, ogl, gameLevel, engine->GetWorld());
            objectOrientedLoader.LoadLevel("../GameLevel_4.txt", "../Models", log);
            objectOrientedLoader.UploadLevelToGPU();

            GLuint shaderProgram = objectOrientedLoader.GetShaderProgram();

            /*IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
          
            io.DisplaySize = ImVec2(800, 600);
          
            ImGui_ImplOpenGL3_Init("#version 130");*/
           
            while (+win.ProcessWindowEvents())
            {
                // Clear the screen
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                /*ImGui_ImplOpenGL3_NewFrame();
                ImGui::NewFrame();*/

                GLint timeLocation = glGetUniformLocation(shaderProgram, "time");
                glUseProgram(shaderProgram);
                glUniform1f(timeLocation, timeElapsed);
                
                objectOrientedLoader.CheckForLevelChange(gameLevel, objectOrientedLoader, engine, kbm, log);

                objectOrientedLoader.RenderLevel();
                
                engine->Update(*gameLevel, log, kbm, 0.1f);

                ogl.UniversalSwapBuffers();

            }         
        }
    }
    /*ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();*/
    return 0; 
}
