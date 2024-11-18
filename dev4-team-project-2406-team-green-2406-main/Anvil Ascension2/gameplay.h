#pragma once
#include "flecs-3.2.0/flecs.h"
#include "components.h"
#include "Physics.h"
//#include "../Anvil Ascension2/imgui-master/backends/imgui_impl_opengl3.h"
#include <cstring> // For memset

class Gameplay {
public:
    std::shared_ptr<flecs::world> world;
    bool ballLaunched = false;
    GW::MATH::GVECTORF ballDirection = { 0.30f, 1.0f, 0.0f, 0.0f }; // Initial direction (upwards)

    GW::MATH::GVECTORF TransformPoint(const GW::MATH::GVECTORF& point, const GW::MATH::GMATRIXF& matrix) {
        GW::MATH::GVECTORF result;
        result.data[0] = point.data[0] * matrix.data[0] + point.data[1] * matrix.data[4] + point.data[2] * matrix.data[8] + point.data[3] * matrix.data[12];
        result.data[1] = point.data[0] * matrix.data[1] + point.data[1] * matrix.data[5] + point.data[2] * matrix.data[9] + point.data[3] * matrix.data[13];
        result.data[2] = point.data[0] * matrix.data[2] + point.data[1] * matrix.data[6] + point.data[2] * matrix.data[10] + point.data[3] * matrix.data[14];
        result.data[3] = point.data[0] * matrix.data[3] + point.data[1] * matrix.data[7] + point.data[2] * matrix.data[11] + point.data[3] * matrix.data[15];
        return result;
    }

    GW::AUDIO::GAudio audioEngine;
    GW::AUDIO::GMusic currentTrack;
    GW::AUDIO::GSound goldDeath;
    GW::AUDIO::GSound launchSound;
    GW::AUDIO::GSound dirtBounce;
    GW::AUDIO::GSound dirtDeath;
    GW::AUDIO::GSound goldBounce;
    GW::AUDIO::GSound anvilBounce;
    GW::AUDIO::GSound gameOver;

    int score = 0;
    int highscore = 0;
    int lifeCounter = 0;

    bool dead = false;
    bool levelcomplete = false;

public:
    Gameplay(const Level_Data & import, GW::SYSTEM::GLog & log) {
        world = std::make_shared<flecs::world>();
        audioEngine.Create();
        currentTrack.Create("../Music/Background.wav", audioEngine, 0.50f);
        currentTrack.Play(true);
        for (auto & i : import.blenderObjects) {
            auto ent = world->entity(i.blendername);
            ent.set<BlenderName>({ i.blendername });

            // Retrieve the OBB for the model
            const GW::MATH::GOBBF & obb = import.levelColliders[import.levelModels[i.modelIndex].colliderIndex];

            // Convert OBB to AABB (Axis-Aligned Bounding Box)
            GW::MATH::GVECTORF minPoint = {
                obb.center.x - obb.extent.x,
                obb.center.y - obb.extent.y,
                obb.center.z - obb.extent.z,
                1.0f
            };
            GW::MATH::GVECTORF maxPoint = {
                obb.center.x + obb.extent.x,
                obb.center.y + obb.extent.y,
                obb.center.z + obb.extent.z,
                1.0f
            };

            // Set the model transform
            GW::MATH::GMATRIXF transform = import.levelTransforms[i.transformIndex];
            GW::MATH::GVECTORF transformedMin = TransformPoint(minPoint, transform);
            GW::MATH::GVECTORF transformedMax = TransformPoint(maxPoint, transform);

            ent.set<ModelBoundary>({ transformedMin, transformedMax });
            ent.set<ModelTransform>({ transform, i.transformIndex });

            std::string modelName = i.blendername;
            modelName = modelName.substr(0, modelName.find_last_of("."));
            if (modelName == "Dirt" || modelName == "Tin") { ent.set<Health>({ 2 }); }
            else if (modelName == "Gold" || modelName == "Emerald") { ent.set<Health>({ 2 }); }
            else if (modelName == "Ruby" || modelName == "Moonstone") { ent.set<Health>({ 3 }); }
            else if (modelName == "Ball") { ent.set<Damage>({ 1 }); }
            else if (modelName == "Player") { ent.set<Lives>({ 4 }); }
        }


        auto f = world->filter<BlenderName, ModelTransform, Lives>();
        f.each([&log](flecs::entity& v, BlenderName& n, ModelTransform& t, Lives& h) {
            std::string obj = "FLECS Entity ";
            obj += n.name + " located at X " + std::to_string(t.matrix.row4.x) +
                " Y " + std::to_string(t.matrix.row4.y) + " Z " + std::to_string(t.matrix.row4.z);
            log.LogCategorized("INFO", obj.c_str());
            });

        auto leftWall = world->lookup("L_Wall");
        auto rightWall = world->lookup("R_Wall");
        auto ceiling = world->lookup("Ceiling");
        auto floor = world->lookup("Floor");
        if (leftWall.is_valid()) {
            const ModelTransform* leftWallTransform = leftWall.get<ModelTransform>();
            const ModelBoundary* leftWallBoundary = leftWall.get<ModelBoundary>();
        }
        if (rightWall.is_valid()) {
            const ModelTransform* rightWallTransform = rightWall.get<ModelTransform>();
            const ModelBoundary* rightWallBoundary = rightWall.get<ModelBoundary>();
        }
        if (ceiling.is_valid()) {
            const ModelTransform* ceilingTransform = ceiling.get<ModelTransform>();
            const ModelBoundary* ceilingBoundary = ceiling.get<ModelBoundary>();
        }
        if (floor.is_valid()) {
            const ModelTransform* floorTransform = floor.get<ModelTransform>();
            const ModelBoundary* floorBoundary = floor.get<ModelBoundary>();
        }
    }

    void ResetBall() {
        auto player = world->lookup("Player");
        auto ball = world->lookup("Ball");
        if (score > 0)
        {
            if (score > highscore)
            {
                highscore = score;
                score = 0;
            }
            else
            {
                score = 0;
            }
        }
        if (player.is_valid() && ball.is_valid()) {
                ballDirection = { 0.30f, 1.0f, 0.0f, 0.0f };
        }
    }


    void updateScore()
    {
        auto ball = world->lookup("Ball");
        if (ball.is_valid())
        {
            auto ballVelocity = ball.get_mut<Velocity>();

            if (ballVelocity != 0)
            {
                score += 0.01f;
            }
        }
    }
    void ResetGame()
    {
        ResetBall();
        auto player = world->lookup("Player");
        player.set<Lives>({ 4 });
    }

    //void setUpScoreOverlay()
    //{
    //    // Set the window to be small, without decorations, and always in the same place
    //    ImGui::SetNextWindowPos(ImVec2(10, 10)); // Position the window at the top-left corner
    //    ImGui::SetNextWindowSize(ImVec2(150, 50)); // Small window size
    //    ImGui::Begin("Integer Display", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);

    //    // Render the integer value
    //    ImGui::Text("Score: %d", score);
    //    ImGui::Text("High Score: %d", highscore);

    //    // End the window
    //    ImGui::End();
    //}

    //void setUpLivesOverlay()
    //{

    //    int windowWidth = 800;
    //    int windowHeight = 600;

    //    static ImVec2 windowPos = ImVec2(690, 540);
    //    ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
    //    static ImVec2 windowSize = ImVec2(100, 50);
    //    ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);
    //    ImGui::Begin("Bottom Right Integer Display", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);

    //    // Render the integer value
    //    auto player = world->lookup("Player");
    //    if (player.get_mut<Lives>()->lives > 0)
    //    {
    //        if (levelcomplete == false)
    //        {
    //            windowPos.x = 690;
    //            ImGui::Text("Lives: %d", (player.get_mut<Lives>()->lives - 1));
    //        }   // If someone can get to this, make it to where this is not hard coded so that we can resize it for extra points
    //        else if (levelcomplete == true)
    //        {
    //            windowSize.x += 100;
    //            windowPos.x = 500;
    //            ImGui::Text("Congrats! Please Exit!");
    //        }
    //    }
    //    else 
    //    {
    //        windowSize.x += 100;
    //        windowPos.x = 500;
    //        ImGui::Text("You have Died! Press R to Reset Level!");
    //    }

    //    // End the window
    //   /* ImGui::End();

    //    ImGui::Render();
    //    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());*/
    //}

    bool kbm = false;
    void Update(Level_Data& level, GW::SYSTEM::GLog& log, GW::INPUT::GInput& input, float dt) {
        // Cap the frame rate
        static auto lastFrameTime = std::chrono::high_resolution_clock::now();
        auto currentFrameTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed = currentFrameTime - lastFrameTime;
        /*setUpScoreOverlay();
        setUpLivesOverlay();*/
        if (elapsed.count() < 1.0f / 60.0f) {
            return; // Return early if frame time is too short
        }
        lastFrameTime = currentFrameTime;

        float kbm_states[256];
        memset(kbm_states, 0, sizeof(kbm_states));

        for (size_t i = 0; i < ARRAYSIZE(kbm_states); ++i) {
            input.GetState(G_KEY_UNKNOWN + i, kbm_states[i]);
        }
        auto player = world->lookup("Player");
        lifeCounter = player.get_mut<Lives>()->lives;
        if (kbm_states[G_KEY_ESCAPE] || dead == true || levelcomplete == true)
        {
            if (kbm_states[G_KEY_R])
            {
                ResetGame();
                dead = false;
            }
            kbm = !kbm;
            return;
        }
        else if (!kbm_states[G_KEY_ESCAPE] && kbm == false)
        {

            // Handle player movement
            float x_change = (kbm_states[G_KEY_D] - kbm_states[G_KEY_A]) * 1.0f; // Adjust speed multiplier if necessary
            GW::MATH::GVECTORF v = GW::MATH::GVECTORF{ x_change * dt, 0.0f, 0.0f, 0.0f };
            ModelTransform* playerTransform = player.get_mut<ModelTransform>();
            ModelBoundary* playerBoundary = player.get_mut<ModelBoundary>();
            if (player.is_valid()) {

                // Tentatively update player position
                float tentativeX = playerTransform->matrix.row4.x + v.x;

                // Check for collisions with wall entities
                auto leftWall = world->lookup("L_Wall");
                auto rightWall = world->lookup("R_Wall");

                if (leftWall.is_valid() && rightWall.is_valid()) {
                    const ModelTransform* leftWallTransform = leftWall.get<ModelTransform>();
                    const ModelTransform* rightWallTransform = rightWall.get<ModelTransform>();
                    const ModelBoundary* leftWallBoundary = leftWall.get<ModelBoundary>();
                    const ModelBoundary* rightWallBoundary = rightWall.get<ModelBoundary>();

                    // Debug: Log wall positions and boundaries
                    // Check and handle collision with left wall (stop at max boundary)
                    if (tentativeX + playerBoundary->maxPoint.x < leftWallBoundary->maxPoint.x) {
                        playerTransform->matrix.row4.x = leftWallBoundary->maxPoint.x - (playerBoundary->maxPoint.x);
                    }
                    // Check and handle collision with right wall (stop at min boundary)
                    else if (tentativeX + playerBoundary->minPoint.x > rightWallBoundary->minPoint.x) {
                        playerTransform->matrix.row4.x = rightWallBoundary->minPoint.x - (playerBoundary->minPoint.x);
                    }
                    else {
                    // No collision, update position
                        playerTransform->matrix.row4.x = tentativeX;
                    }
                }

                // Log position after potential update
            }

            // Handle ball movement
            auto ball = world->lookup("Ball");
            auto floor = world->lookup("Floor");
            const float gravity = -0.01f;
            if (ball.is_valid()) {
                ModelTransform* ballTransform = ball.get_mut<ModelTransform>();
                ModelBoundary* ballBoundary = ball.get_mut<ModelBoundary>();
                float originalTransform = player.get<ModelTransform>()->matrix.row4.y;
                if (!ballLaunched) {
                    // Position ball with the player
                    ballTransform->matrix.row4 = player.get<ModelTransform>()->matrix.row4;
                    ballTransform->matrix.row4.y += 0.25f; // Position slightly above the player
                    if (kbm_states[G_KEY_SPACE]) {
                        ballLaunched = true;
                        launchSound.Create("../SoundFX/LaunchBall.wav", audioEngine, 0.35f);
                        launchSound.Play();
                    }
                }
                else {
                    ballDirection.y += gravity * dt;
                    // Move the ball
                    ballTransform->matrix.row4.x += ballDirection.x * dt;
                    ballTransform->matrix.row4.y += ballDirection.y * dt;

                    // Update ball boundary points
                    ballBoundary->minPoint.x += ballDirection.x * dt;
                    ballBoundary->maxPoint.x += ballDirection.x * dt;
                    ballBoundary->minPoint.y += ballDirection.y * dt;
                    ballBoundary->maxPoint.y += ballDirection.y * dt;

                    // Check for collisions with walls
                    auto leftWall = world->lookup("L_Wall");
                    auto rightWall = world->lookup("R_Wall");
                    auto ceiling = world->lookup("Ceiling");
                    //auto floor = world->lookup("Floor");
                    auto dirt = world->lookup("Dirt");
                    auto gold = world->lookup("Gold");

                    if (leftWall.is_valid() && rightWall.is_valid() && ceiling.is_valid() && floor.is_valid()) {
                        const ModelBoundary* leftWallBoundary = leftWall.get<ModelBoundary>();
                        const ModelBoundary* rightWallBoundary = rightWall.get<ModelBoundary>();
                        const ModelBoundary* ceilingBoundary = ceiling.get<ModelBoundary>();
                        const ModelBoundary* floorBoundary = floor.get<ModelBoundary>();
                        GW::MATH::GVECTORF v = GW::MATH::GVECTORF{ x_change * dt, 0.0f, 0.0f, 0.0f };
                       
                        if (ballTransform->matrix.row4.x < leftWallBoundary->maxPoint.x) {
                            ballDirection.x = std::abs(ballDirection.x); // Bounce right
                            dirtBounce.Create("../SoundFX/RockHittingDirt.wav", audioEngine, 0.35f);
                            dirtBounce.Play();
                        }
                        else if (ballTransform->matrix.row4.x > rightWallBoundary->minPoint.x) {
                            ballDirection.x = -std::abs(ballDirection.x); // Bounce left
                            dirtBounce.Create("../SoundFX/RockHittingDirt.wav", audioEngine, 0.35f);
                            dirtBounce.Play();
                        }

                        // Check for collision with ceiling and floor
                        if (ballBoundary->maxPoint.y > ceilingBoundary->minPoint.y) {
                            ballDirection.y = -std::abs(ballDirection.y); // Bounce down
                            dirtBounce.Create("../SoundFX/RockHittingDirt.wav", audioEngine, 0.35f);
                            dirtBounce.Play();
                        }

                        if ((ballBoundary->minPoint.y = playerTransform->matrix.row4.y) && !(ballTransform->matrix.row4.y > playerTransform->matrix.row4.y))
                        {
                            if (((playerTransform->matrix.row4.x - 0.40f) > ballTransform->matrix.row4.x) || ((playerTransform->matrix.row4.x + 0.40f) < ballTransform->matrix.row4.x))
                            {
                                if (ballTransform->matrix.row4.y <= 0.90f) //floorBoundary->maxPoint.y //Change the y value it resets at.
                                {
                                    ballDirection = { 0.30f, 1.0f, 0.0f, 0.0f };
                                    player.set<Lives>({ lifeCounter - 1 });
                                    ballLaunched = !ballLaunched; // Reset ball position
                                }
                            }
                            else if (((playerTransform->matrix.row4.x - 0.40f) <= ballTransform->matrix.row4.x) || ((playerTransform->matrix.row4.x + 0.40f) >= ballTransform->matrix.row4.x))
                            {
                                anvilBounce.Create("../SoundFX/anvilhit.wav", audioEngine, 0.15f);
                                anvilBounce.Play();
                                ballDirection.y = std::abs(ballDirection.y);
                                score += 100;
                            }
                            else
                            {
                                ballDirection.y = ballDirection.y;
                            }
                        }

                    }

                    if (dirt.is_valid())
                    {

                        const ModelBoundary* dirtBoundary = dirt.get<ModelBoundary>();
                        int dirtHealth = dirt.get_mut<Health>()->health;
                        if (ballBoundary->maxPoint.y > dirtBoundary->minPoint.y) {
                            ballDirection.y = -std::abs(ballDirection.y); // Bounce down
                            dirt.set<Health>({ dirtHealth - 1 });
                            dirtBounce.Create("../SoundFX/RockHittingDirt.wav", audioEngine, 0.35f);
                            dirtBounce.Play();
                            score += 50;
                            if (dirtHealth == 0)
                            {
                                goldDeath.Create("../SoundFX/GoldFalling.wav", audioEngine, 0.35f);
                                goldDeath.Play();
                                dirt.destruct();
                                score += 200;
                            }
                        }                    

                    }

                    if (gold.is_valid())
                    {
                        const ModelBoundary* goldBoundary = gold.get<ModelBoundary>();
                        int goldHealth = gold.get_mut<Health>()->health;
                        if (ballBoundary->maxPoint.y > goldBoundary->minPoint.y) {
                            ballDirection.y = -std::abs(ballDirection.y); // Bounce down
                            goldBounce.Create("../SoundFX/GoldHit.wav", audioEngine, 0.35f);
                            goldBounce.Play();
                            score += 100;
                            if (goldHealth == 2)
                            {
                                gold.set<Health>({ goldHealth - 1 });
                            }
                            else if (goldHealth == 1)
                            {
                                gold.set<Health>({ goldHealth - 1 });
                            }
                            else if (goldHealth == 0)
                            {
                                goldDeath.Create("../SoundFX/GoldFalling.wav", audioEngine, 0.35f);
                                goldDeath.Play();
                                gold.destruct();                                
                                score += 1000;
                            }
                        }
                    }
                    if (!gold.is_valid() && !dirt.is_valid())
                    {
                        levelcomplete = true;
                    }
                }


                if (lifeCounter == 0)
                {
                    gameOver.Create("../SoundFX/GameOver.wav", audioEngine, 0.35f);
                    gameOver.Play();
                    dead = true;
                }

                // Log ball position after update
            }
        }
    }

    bool CheckCollision(const ModelBoundary& a, const ModelBoundary& b) {
        return (a.minPoint.x <= b.maxPoint.x && a.maxPoint.x >= b.minPoint.x) &&
            (a.minPoint.y <= b.maxPoint.y && a.maxPoint.y >= b.minPoint.y) &&
            (a.minPoint.z <= b.maxPoint.z && a.maxPoint.z >= b.minPoint.z);
    }

    std::shared_ptr<flecs::world> GetWorld() const {
        return world;
    }

};
